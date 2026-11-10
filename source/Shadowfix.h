#pragma once
#ifndef NFSMW_WIDESCREENFIX_SHADOWFIX_H
#define NFSMW_WIDESCREENFIX_SHADOWFIX_H

#include <windows.h>
#include <cstdint>
#include "MemoryTools.h"
#include "Resolution.h"
#include "Config.h"

namespace ShadowFix {
    // Verified from IDA: Shadow rendering pipeline (sub_6DE300, sub_6C8770, sub_6C1510)

    // --- Dynamic Shadow Tunnel / Bridge Fix (0x006DE300 & 0x00744468) ---
    constexpr uintptr_t ADDR_SHADOW_FADE_JNZ     = 0x006DE377; // 2 bytes: 75 3B -> NOP
    constexpr uintptr_t ADDR_SHADOW_FADE_FLOAT   = 0x006DE37F; // Immediate float 1.0f -> 0.0f
    constexpr uintptr_t ADDR_CAR_SHADOW_OPACITY  = 0x008A0E50; // Dynamic car ambient shadow opacity (110.0f -> 60.0f)

    // --- Dynamic Shadow Resolution Texture Operands (sub_6C8770) ---
    // Width (X) operands
    constexpr uintptr_t ADDR_SHADOW_RES_X_MOV    = 0x006C86B1; // mov eax, Width (operand at 0x6C86B0 + 1)
    constexpr uintptr_t ADDR_SHADOW_RES_X_PUSH1  = 0x006C878C; // push Width (operand at 0x6C878B + 1)
    constexpr uintptr_t ADDR_SHADOW_RES_X_PUSH2  = 0x006C87BE; // push Width (operand at 0x6C87BD + 1)
    constexpr uintptr_t ADDR_SHADOW_RES_X_PUSH3  = 0x006C87F4; // push Width (operand at 0x6C87F3 + 1)

    // Height (Y) operands
    constexpr uintptr_t ADDR_SHADOW_RES_Y_MOV    = 0x006C86C1; // mov eax, Height (operand at 0x6C86C0 + 1)
    constexpr uintptr_t ADDR_SHADOW_RES_Y_PUSH1  = 0x006C8787; // push Height (operand at 0x6C8786 + 1)
    constexpr uintptr_t ADDR_SHADOW_RES_Y_PUSH2  = 0x006C87B9; // push Height (operand at 0x6C87B8 + 1)
    constexpr uintptr_t ADDR_SHADOW_RES_Y_PUSH3  = 0x006C87EF; // push Height (operand at 0x6C87EE + 1)

    // --- Depth Bias & Slope Scaled Bias (sub_6E54E0) ---
    constexpr uintptr_t ADDR_DEPTH_BIAS_INST     = 0x006E54E1; // fild dword_901AC0 (6 bytes: DB 05 C0 1A 90 00)
    constexpr uintptr_t ADDR_DEPTH_BIAS_SLOPE    = 0x006E5507; // mov edx, dword_901ABC (6 bytes: 8B 15 BC 1A 90 00)
    constexpr uintptr_t ADDR_VANILLA_DEPTH_BIAS  = 0x00901AC0; // int32_t vanilla depth bias (4)
    constexpr uintptr_t ADDR_VANILLA_DEPTH_SLOPE = 0x00901ABC; // float vanilla depth bias slope (4.0f)

    // --- Shadow Level of Detail (LOD) Enhancements ---
    constexpr uintptr_t ADDR_SHADOW_LOD_CULL     = 0x006E5174; // push 200h (LOD culling flag) -> push 0
    constexpr uintptr_t ADDR_SHADOW_LOD_DETAIL   = 0x006BFFA2; // push 4100h (geometry flags) -> push 6102h

    // --- Texture Formats (D24S8, INTZ, DF16, DF24) ---
    constexpr uintptr_t ADDR_CREATE_FORMAT_CHECK     = 0x006C8798;
    constexpr uintptr_t ADDR_CREATE_D24S8_BRANCH     = 0x006C87A2;
    constexpr uintptr_t ADDR_CREATE_D24S8_PUSH       = 0x006C87B2; // push 4Bh (75 = D24S8)
    constexpr uintptr_t ADDR_CREATE_FOURCC_BRANCH    = 0x006C87D5;
    constexpr uintptr_t ADDR_CREATE_FOURCC_PUSH      = 0x006C87E5; // push 'DF16' (FOURCC)

    constexpr uintptr_t ADDR_CHECK_DEV_START         = 0x006C1703; // mov dword_982C08, ebx (6 bytes)
    constexpr uintptr_t ADDR_CHECK_DEV_JNZ           = 0x006C170A; // jnz loc_6C173A in sub_6C1510 (2 bytes)
    constexpr uintptr_t ADDR_CHECK_DEV_D24S8_PUSH    = 0x006C1719; // push 4Bh
    constexpr uintptr_t ADDR_CHECK_DEV_SET_1         = 0x006C172C; // mov dword_982C08, 1 (10 bytes)
    constexpr uintptr_t ADDR_CHECK_DEV_FOURCC_BRANCH = 0x006C1741;
    constexpr uintptr_t ADDR_CHECK_DEV_FOURCC_PUSH   = 0x006C174E; // push 'DF16'
    constexpr uintptr_t ADDR_CHECK_DEV_SET_2         = 0x006C1764; // mov dword_982C08, 2 (10 bytes)
    constexpr uintptr_t ADDR_SHADOW_TYPE_GLOBAL      = 0x00982C08; // 1 = D24S8, 2 = Depth Buffer

    // --- Force Sharp Shadows ---
    constexpr uintptr_t ADDR_FORCE_SHARP_SUB         = 0x006D5F3A; // sub eax, 3 -> sub eax, 2

    // FourCC and enum constants
    constexpr int kFmt_D24S8 = 75;
    constexpr int kFmt_INTZ  = 0x5A544E49; // 'INTZ'
    constexpr int kFmt_DF16  = 0x36314644; // 'DF16'
    constexpr int kFmt_DF24  = 0x34324644; // 'DF24'

    inline uint32_t s_BaseResolution = 2048;
    inline bool s_AutoScale = true;
    inline int s_ConfigFormat = 0; // 0 = Auto
    inline bool s_ForceSharpShadows = false;

    inline int s_ActiveFormat = kFmt_INTZ;
    inline int s_ActiveMode = 2; // 1 = Direct Shadow Map, 2 = Depth Buffer as Texture

    inline float s_DepthBias = 4.0f;
    inline float s_DepthBiasSlope = 4.0f;

    inline void UpdateShadowsRes(float aspectRatio) {
        if (s_BaseResolution == 0) return;

        uint32_t resval = s_BaseResolution;
        if (resval > 16384) resval = 16384;
        if (resval < 512) resval = 512;

        uint32_t resX = resval;
        if (s_AutoScale) {
            float newShadowX = static_cast<float>(resval) * (aspectRatio / (4.0f / 3.0f));
            resX = static_cast<uint32_t>(newShadowX);
        }
        uint32_t resY = resval;

        if (resX > 16384) resX = 16384;

        // Patch Width (X) operands
        MemoryTools::Write<uint32_t>(ADDR_SHADOW_RES_X_MOV,   resX);
        MemoryTools::Write<uint32_t>(ADDR_SHADOW_RES_X_PUSH1, resX);
        MemoryTools::Write<uint32_t>(ADDR_SHADOW_RES_X_PUSH2, resX);
        MemoryTools::Write<uint32_t>(ADDR_SHADOW_RES_X_PUSH3, resX);

        // Patch Height (Y) operands
        MemoryTools::Write<uint32_t>(ADDR_SHADOW_RES_Y_MOV,   resY);
        MemoryTools::Write<uint32_t>(ADDR_SHADOW_RES_Y_PUSH1, resY);
        MemoryTools::Write<uint32_t>(ADDR_SHADOW_RES_Y_PUSH2, resY);
        MemoryTools::Write<uint32_t>(ADDR_SHADOW_RES_Y_PUSH3, resY);

        if (resX > resval) resval = resX;

        // Calculate dynamic depth bias scaling
        if (s_ActiveMode == 2 && !s_ForceSharpShadows) {
            s_DepthBias = static_cast<float>(MemoryTools::Read<int32_t>(ADDR_VANILLA_DEPTH_BIAS)) * (static_cast<float>(resval) / 1024.0f);
            s_DepthBiasSlope = MemoryTools::Read<float>(ADDR_VANILLA_DEPTH_SLOPE) * (static_cast<float>(resval) / 1024.0f);
        } else if (resval > 3072) {
            s_DepthBias = static_cast<float>(MemoryTools::Read<int32_t>(ADDR_VANILLA_DEPTH_BIAS)) * (static_cast<float>(resval) / 3072.0f);
            s_DepthBiasSlope = MemoryTools::Read<float>(ADDR_VANILLA_DEPTH_SLOPE) * (static_cast<float>(resval) / 3072.0f);
        } else {
            s_DepthBias = static_cast<float>(MemoryTools::Read<int32_t>(ADDR_VANILLA_DEPTH_BIAS));
            s_DepthBiasSlope = MemoryTools::Read<float>(ADDR_VANILLA_DEPTH_SLOPE);
        }
    }

    inline void OnResolutionChange(int width, int height) {
        if (width <= 0 || height <= 0) return;
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        UpdateShadowsRes(aspect);
    }

    inline void ApplyTextureFormat(int formatOption) {
        if (formatOption < 0) return;

        // Auto (0) selects D24S8 when Sunset mod is installed to maintain exhaust flame
        // quad rendering in Sunset's custom particle shaders, otherwise INTZ for universal modern GPUs.
        uint32_t chosenFormat = g_Config.SunSetInstalled ? kFmt_D24S8 : kFmt_INTZ;
        if (formatOption == 1) chosenFormat = kFmt_D24S8;
        else if (formatOption == 2) chosenFormat = kFmt_INTZ;
        else if (formatOption == 3) chosenFormat = kFmt_DF16;
        else if (formatOption == 4) chosenFormat = kFmt_DF24;

        s_ActiveFormat = chosenFormat;

        // Disable vanilla hardcoded writes to dword_982C08
        MemoryTools::MakeNOP(ADDR_CHECK_DEV_START, 6);
        MemoryTools::MakeNOP(ADDR_CHECK_DEV_SET_1, 10);
        MemoryTools::MakeNOP(ADDR_CHECK_DEV_SET_2, 10);

        if (chosenFormat < 0x7F) {
            // D24S8 format (Direct Shadow Map, sharp)
            MemoryTools::MakeJMP(ADDR_CREATE_FORMAT_CHECK, ADDR_CREATE_D24S8_BRANCH);
            MemoryTools::Write<uint8_t>(ADDR_CREATE_D24S8_PUSH + 1, static_cast<uint8_t>(chosenFormat & 0xFF));

            MemoryTools::MakeNOP(ADDR_CHECK_DEV_JNZ, 2);
            MemoryTools::Write<uint8_t>(ADDR_CHECK_DEV_D24S8_PUSH + 1, static_cast<uint8_t>(chosenFormat & 0xFF));

            *reinterpret_cast<uint32_t*>(ADDR_SHADOW_TYPE_GLOBAL) = 1;
            s_ActiveMode = 1;
        } else {
            // FOURCC format (INTZ, DF16, DF24 - Depth Buffer as Texture, soft)
            MemoryTools::MakeJMP(ADDR_CREATE_FORMAT_CHECK, ADDR_CREATE_FOURCC_BRANCH);
            MemoryTools::Write<uint32_t>(ADDR_CREATE_FOURCC_PUSH + 1, chosenFormat);

            MemoryTools::MakeJMP(ADDR_CHECK_DEV_JNZ, ADDR_CHECK_DEV_FOURCC_BRANCH);
            MemoryTools::Write<uint32_t>(ADDR_CHECK_DEV_FOURCC_PUSH + 1, chosenFormat);

            *reinterpret_cast<uint32_t*>(ADDR_SHADOW_TYPE_GLOBAL) = 2;
            s_ActiveMode = 2;
        }
    }

    inline void Init(int shadowsRes, int shadowsFix, int improveShadowLOD, int autoScaleShadowsRes, int shadowMapTextureFormat, int forceSharpShadows) {
        // 1. Dynamic Shadows Disappearing Fix (Tunnels / Bridges)
        if (shadowsFix != 0) {
            // NOP the jump at 0x006DE377 so shadow fading is always bypassed
            MemoryTools::MakeNOP(ADDR_SHADOW_FADE_JNZ, 2);
            // Write 0.0f as immediate float to keep flt_982BA8 at 0 (full visibility)
            MemoryTools::Write<float>(ADDR_SHADOW_FADE_FLOAT, 0.0f);
            // Soften dynamic ambient car shadow from 110.0f to 60.0f
            MemoryTools::Write<float>(ADDR_CAR_SHADOW_OPACITY, 60.0f);
        }

        // 2. Texture Format (Auto / INTZ / D24S8 / etc.)
        s_ConfigFormat = shadowMapTextureFormat;
        s_ForceSharpShadows = (forceSharpShadows != 0);
        ApplyTextureFormat(shadowMapTextureFormat);

        // 3. Dynamic Shadow Resolution & Auto-Scale
        s_BaseResolution = (shadowsRes > 0) ? static_cast<uint32_t>(shadowsRes) : 2048;
        s_AutoScale = (autoScaleShadowsRes != 0);

        if (shadowsRes > 0) {
            // Hook depth bias in sub_6E54E0:
            // Convert 'fild dword ptr [0x00901AC0]' (DB 05 ...) into 'fld dword ptr [&s_DepthBias]' (D9 05 ...)
            uint8_t fldOpcode[2] = { 0xD9, 0x05 };
            MemoryTools::WriteBytes(ADDR_DEPTH_BIAS_INST, fldOpcode, 2);
            MemoryTools::Write<const float*>(ADDR_DEPTH_BIAS_INST + 2, &s_DepthBias);

            // Redirect 'mov edx, [0x00901ABC]' to read from &s_DepthBiasSlope
            MemoryTools::Write<const float*>(ADDR_DEPTH_BIAS_SLOPE + 2, &s_DepthBiasSlope);

            // Register callback with Resolution system so resolution / aspect changes update shadow dimensions
            Resolution::RegisterOnResChange(OnResolutionChange);

            // Initial resolution update
            UpdateShadowsRes(Resolution::g_CurrentAspectRatio);
        }

        // 4. Force Sharp Shadows (Stencil technique override on INTZ/DF16/DF24)
        if (forceSharpShadows != 0) {
            // sub eax, 3 -> sub eax, 2
            MemoryTools::Write<uint8_t>(ADDR_FORCE_SHARP_SUB + 2, 2);
        }

        // 5. Improve Shadow Level of Detail (LOD)
        if (improveShadowLOD != 0) {
            // Disable shadow LOD distance culling: push 200h -> push 0
            MemoryTools::Write<uint32_t>(ADDR_SHADOW_LOD_CULL, 0x00000000);
            // Enable high-detail geometry flags in shadow pass: push 4100h -> push 6102h
            MemoryTools::Write<uint32_t>(ADDR_SHADOW_LOD_DETAIL, 0x00006102);
        }
    }
}

#endif // NFSMW_WIDESCREENFIX_SHADOWFIX_H
