#pragma once
#ifndef NFSMW_WIDESCREENFIX_SIMRATE_H
#define NFSMW_WIDESCREENFIX_SIMRATE_H

#include <windows.h>
#include <mmsystem.h>
#include <cstdint>
#include "MemoryTools.h"

#pragma comment(lib, "winmm.lib")

namespace SimRate {
    // Verified from IDA: Engine timestep addresses (sub_6612B0, sub_6660E0, sub_65C780)

    // Engine addresses verified for speed.exe v1.3
    constexpr uintptr_t ADDR_FLT_8970F0        = 0x008970F0; // Static float constant in .rdata (default 0.016666668f)
    constexpr uintptr_t ADDR_REALTIMESTEP_PTR  = 0x006612EC; // PrepareRealTimestep: pointer operand to flt_8970F0
    constexpr uintptr_t ADDR_INIT_TIMESTEP     = 0x00666100; // InitializeEverything: immediate float push for Scheduler::Init
    constexpr uintptr_t ADDR_WORLD_TIMESTEP    = 0x00903290; // World_Service: float global initial timestep state
    constexpr uintptr_t ADDR_DEBUG_TIME_PTR1   = 0x0065C78F; // GetDebugRealTime: first pointer operand to flt_8970F0
    constexpr uintptr_t ADDR_DEBUG_TIME_PTR2   = 0x0065C7D9; // GetDebugRealTime: second pointer operand to flt_8970F0

    // FE Garage 60 FPS Cap addresses
    constexpr uintptr_t ADDR_MAIN_ANIMATE_CALL        = 0x00663E07; // MainLoop - call Main_AnimateFrame__Ff (E8 74 68 FE FF)
    constexpr uintptr_t ADDR_FUNC_MAIN_ANIMATE_FRAME  = 0x0064A680; // Main_AnimateFrame(float simDelta)
    constexpr uintptr_t ADDR_GAMEFLOW_MANAGER         = 0x00925E90; // TheGameFlowManager (3 = Garage / Safehouse / Shop, 6 = Freeroam / Racing)

    inline uint8_t s_OrigMainAnimateCallBytes[5] = { 0xE8, 0x74, 0x68, 0xFE, 0xFF };
    inline bool s_GarageCapHooked = false;

    inline float s_SimFrameTime = 1.0f / 200.0f; // Default 200 Hz (0.005f)
    inline float s_SimHz = 200.0f;
    inline bool s_SimRateEnabled = false;

    // High-Precision Hybrid Frame Pacer (Sleep + QPC Spin-wait) for FE Garage
    inline void ApplyFrameLimit(float targetFps) {
        if (targetFps <= 1.0f) return;

        static LARGE_INTEGER s_Frequency = { 0 };
        static LARGE_INTEGER s_LastTime = { 0 };

        if (s_Frequency.QuadPart == 0) {
            QueryPerformanceFrequency(&s_Frequency);
            QueryPerformanceCounter(&s_LastTime);
            return;
        }

        const double targetInterval = 1.0 / static_cast<double>(targetFps);
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        double elapsed = static_cast<double>(currentTime.QuadPart - s_LastTime.QuadPart) / static_cast<double>(s_Frequency.QuadPart);

        double remaining = targetInterval - elapsed;
        if (remaining > 0.002) {
            DWORD sleepMs = static_cast<DWORD>((remaining - 0.0015) * 1000.0);
            if (sleepMs > 0) {
                Sleep(sleepMs);
            }
        }

        while (true) {
            QueryPerformanceCounter(&currentTime);
            elapsed = static_cast<double>(currentTime.QuadPart - s_LastTime.QuadPart) / static_cast<double>(s_Frequency.QuadPart);
            if (elapsed >= targetInterval) {
                break;
            }
            YieldProcessor();
        }

        s_LastTime = currentTime;
    }

    // MainLoop Animation Dispatch Hook: locks FE Garage to 60 FPS
    inline float __cdecl Custom_Main_AnimateFrame(float simDelta) {
        const int gameFlow = *reinterpret_cast<const int*>(ADDR_GAMEFLOW_MANAGER);

        // FE Garage / Safehouse / Car Lot / Customization Shop (gameflow == 3):
        // Lock strictly to 60 FPS cap so garage cameras, turntable, and UI run at native 60 FPS!
        if (gameFlow == 3) {
            ApplyFrameLimit(60.0f);
        }

        return reinterpret_cast<float(__cdecl*)(float)>(ADDR_FUNC_MAIN_ANIMATE_FRAME)(simDelta);
    }

    inline int GetDesktopRefreshRate() {
        DEVMODEA dm = { 0 };
        dm.dmSize = sizeof(dm);
        if (EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm) && dm.dmDisplayFrequency >= 60) {
            return static_cast<int>(dm.dmDisplayFrequency);
        }
        return 60;
    }

    inline void Init(bool enable, int configRate = 1, bool lockGarageFps = true) {
        // Activate high-precision 1ms Windows timer resolution
        timeBeginPeriod(1);

        // FE Garage 60 FPS Cap: Enabled by default, can be toggled via INI
        if (lockGarageFps && !s_GarageCapHooked) {
            MemoryTools::MakeCALL(ADDR_MAIN_ANIMATE_CALL, reinterpret_cast<uintptr_t>(Custom_Main_AnimateFrame));
            s_GarageCapHooked = true;
        }

        if (!enable) return;

        // Determine simulation frequency (Hz)
        // 1 = Dynamic (Matches monitor refresh rate, default)
        // 2 = Default 200 Hz Decoupled Physics
        // > 2 = Custom Hz if explicitly specified
        float targetHz = 200.0f;
        if (configRate == 1) {
            targetHz = static_cast<float>(GetDesktopRefreshRate());
        } else if (configRate == 2) {
            targetHz = 200.0f;
        } else if (configRate > 2) {
            targetHz = static_cast<float>(configRate);
        }

        if (targetHz < 60.0f) targetHz = 60.0f;
        if (targetHz > 500.0f) targetHz = 500.0f;

        s_SimHz = targetHz;
        s_SimFrameTime = 1.0f / targetHz;

        s_SimRateEnabled = true;

        // 1. Overwrite static float in .rdata so all default references pick up target Hz
        MemoryTools::Write<float>(ADDR_FLT_8970F0, s_SimFrameTime);

        // 2. PrepareRealTimestep (0x00661280): redirect pointer operand to our stable FrameTime
        MemoryTools::Write<const float*>(ADDR_REALTIMESTEP_PTR, &s_SimFrameTime);

        // 3. InitializeEverything (0x00665FC0): patch immediate float argument for Scheduler::Init
        MemoryTools::Write<float>(ADDR_INIT_TIMESTEP, s_SimFrameTime);

        // 4. World_Service (0x0075AAD0): patch initial timestep state in .data
        MemoryTools::Write<float>(ADDR_WORLD_TIMESTEP, s_SimFrameTime);

        // 5. GetDebugRealTime (0x0065C770): redirect pointer operands to our stable FrameTime
        MemoryTools::Write<const float*>(ADDR_DEBUG_TIME_PTR1, &s_SimFrameTime);
        MemoryTools::Write<const float*>(ADDR_DEBUG_TIME_PTR2, &s_SimFrameTime);
    }

    inline void Shutdown() {
        if (s_GarageCapHooked) {
            MemoryTools::WriteBytes(ADDR_MAIN_ANIMATE_CALL, s_OrigMainAnimateCallBytes, 5);
            s_GarageCapHooked = false;
        }

        if (s_SimRateEnabled) {
            s_SimRateEnabled = false;
        }

        timeEndPeriod(1);
    }
}

#endif // NFSMW_WIDESCREENFIX_SIMRATE_H
