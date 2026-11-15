#pragma once
#ifndef NFSMW_WIDESCREENFIX_NOSTRAILFIX_H
#define NFSMW_WIDESCREENFIX_NOSTRAILFIX_H

#include <windows.h>
#include <cstdint>
#include <cstring>
#include "MemoryTools.h"

namespace NOSTrailFix {
    // Verified from IDA: VehicleRenderConn::RenderFlares (0x00744E20)
    constexpr uintptr_t ADDR_NOS_TRAIL_TEST   = 0x00744FB7; // 85 C0 (test eax, eax)
    constexpr uintptr_t ADDR_NOS_TRAIL_LENGTH = 0x00745038; // 8 bytes: EB 06 8D 9B 00 00 00 00
    constexpr uintptr_t ADDR_NOS_TRAIL_CALL   = 0x007450E8; // call CarRenderInfo::RenderFlaresOnCar

    // Pause Fix addresses (VehicleRenderConn::RenderFlares & FEManager::mPauseRequest)
    inline const uintptr_t ADDR_PAUSE_REQUEST    = 0x0091CAE4; // FEManager::mPauseRequest
    inline const uintptr_t ADDR_HISTORY_HOOK     = 0x00744F0C; // cmp [edi+1280h], ecx (6 bytes: 39 8F 80 12 00 00)
    inline const uintptr_t RET_HISTORY_NORMAL    = 0x00744F12; // jge loc_744F1A
    inline const uintptr_t JMP_HISTORY_SKIP      = 0x00744F79; // loc_744F79

    inline const uintptr_t ADDR_SPEED_CHECK_HOOK = 0x00744FA7; // jnz loc_74510E (6 bytes: 0F 85 61 01 00 00)
    inline const uintptr_t RET_SPEED_CONTINUE    = 0x00744FAD; // mov edx, [esp+10h]
    inline const uintptr_t JMP_SPEED_SKIP        = 0x0074510E; // loc_74510E

    struct bVector3 {
        float x, y, z;
    };

    struct bVector4 {
        float x, y, z, w;
    };

    struct bMatrix4 {
        bVector4 v0;
        bVector4 v1;
        bVector4 v2;
        bVector4 v3;
    };

    inline float s_NOSTrailLength = 0.0f;
    inline float s_NOSTrailPositionScalar = 0.0f;

    // Hook at 0x00745038: scales delta vector in [esp+30h], [esp+34h], [esp+38h]
    __declspec(naked) inline void Hook_NOSTrailLength() {
        __asm {
            // [esp+4] is the return address pushed by call
            fld dword ptr [esp + 0x30 + 4]
            fmul ds:[s_NOSTrailLength]
            fstp dword ptr [esp + 0x30 + 4]

            fld dword ptr [esp + 0x34 + 4]
            fmul ds:[s_NOSTrailLength]
            fstp dword ptr [esp + 0x34 + 4]

            fld dword ptr [esp + 0x38 + 4]
            fmul ds:[s_NOSTrailLength]
            fstp dword ptr [esp + 0x38 + 4]

            xor eax, eax
            ret
        }
    }

    // Original RenderFlaresOnCar function pointer
    using PFN_RenderFlaresOnCar = void(__thiscall*)(void* thisCarRenderInfo, void* pView, bVector3* pPosition, bMatrix4* pBodyMatrix, int flag1, int flag2, int flag3);
    inline PFN_RenderFlaresOnCar s_OrigRenderFlaresOnCar = nullptr;

    inline void __fastcall Hook_RenderFlaresOnCar(void* thisCarRenderInfo, void* dummyEdx, void* pView, bVector3* pPosition, bMatrix4* pBodyMatrix, int flag1, int flag2, int flag3) {
        if (s_NOSTrailPositionScalar <= 0.0f || !pBodyMatrix || !s_OrigRenderFlaresOnCar) {
            if (s_OrigRenderFlaresOnCar) {
                s_OrigRenderFlaresOnCar(thisCarRenderInfo, pView, pPosition, pBodyMatrix, flag1, flag2, flag3);
            }
            return;
        }

        bMatrix4 adjustedMatrix = *pBodyMatrix;

        float posScale = 1.0f + s_NOSTrailPositionScalar;

        adjustedMatrix.v0.x *= posScale;
        adjustedMatrix.v0.y *= posScale;
        adjustedMatrix.v0.z *= posScale;

        adjustedMatrix.v2.x *= posScale;
        adjustedMatrix.v2.y *= posScale;

        s_OrigRenderFlaresOnCar(thisCarRenderInfo, pView, pPosition, &adjustedMatrix, flag1, flag2, flag3);
    }

    // Hook at 0x00744F0C: Freezes the 3-frame history ring buffer while paused so previous positions are preserved
    __declspec(naked) inline void Hook_HistoryBufferPause() {
        __asm {
            cmp dword ptr ds:[0x0091CAE4], 0
            jne skip_history

            // Original instruction: cmp [edi+1280h], ecx
            cmp dword ptr [edi + 0x1280], ecx
            jmp dword ptr [RET_HISTORY_NORMAL]

        skip_history:
            jmp dword ptr [JMP_HISTORY_SKIP]
        }
    }

    // Hook at 0x00744FA7: Bypasses velocity check when paused so trail loop still renders frozen flare quads
    __declspec(naked) inline void Hook_SpeedCheckPause() {
        __asm {
            // Evaluate flags from original (test ah, 41h at 0x00744FA4)
            jnz speed_below_threshold
            // Speed > 0.2f: proceed to trail rendering
            jmp dword ptr [RET_SPEED_CONTINUE]

        speed_below_threshold:
            // Speed <= 0.2f. If paused, keep rendering trail!
            cmp dword ptr ds:[0x0091CAE4], 0
            jne continue_paused

            // Not paused: skip trail rendering
            jmp dword ptr [JMP_SPEED_SKIP]

        continue_paused:
            jmp dword ptr [RET_SPEED_CONTINUE]
        }
    }

    inline void Init(bool nosTrailEnabled, float trailLength = 0.0f, float positionScalar = 0.0f, bool pauseFix = false) {
        // Toggle: 0x00744FB7 (31 C0 = xor eax, eax skips trail; 85 C0 = test eax, eax) - Verified from IDA
        if (!nosTrailEnabled) {
            MemoryTools::Write<uint16_t>(ADDR_NOS_TRAIL_TEST, 0xC031);
            return;
        }

        MemoryTools::Write<uint16_t>(ADDR_NOS_TRAIL_TEST, 0xC085);

        s_NOSTrailLength = trailLength;
        s_NOSTrailPositionScalar = positionScalar;

        // Custom Length: 0x00745038 (only hooked if custom length > 0) - Verified from IDA
        if (s_NOSTrailLength > 0.0f) {
            MemoryTools::MakeCALL(ADDR_NOS_TRAIL_LENGTH, reinterpret_cast<uintptr_t>(Hook_NOSTrailLength), 3);
        }

        // Custom Position: 0x007450E8 (only hooked if position scalar > 0) - Verified from IDA
        if (s_NOSTrailPositionScalar > 0.0f) {
            s_OrigRenderFlaresOnCar = reinterpret_cast<PFN_RenderFlaresOnCar>(MemoryTools::GetCallTarget(ADDR_NOS_TRAIL_CALL));
            MemoryTools::MakeCALL(ADDR_NOS_TRAIL_CALL, reinterpret_cast<uintptr_t>(Hook_RenderFlaresOnCar));
        }

        // Pause Fix: Freeze history ring buffer and bypass speed check during pause
        if (pauseFix) {
            MemoryTools::MakeJMP(ADDR_HISTORY_HOOK, reinterpret_cast<uintptr_t>(Hook_HistoryBufferPause), 1);
            MemoryTools::MakeJMP(ADDR_SPEED_CHECK_HOOK, reinterpret_cast<uintptr_t>(Hook_SpeedCheckPause), 1);
        }
    }
}

#endif // NFSMW_WIDESCREENFIX_NOSTRAILFIX_H
