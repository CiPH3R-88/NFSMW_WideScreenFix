#pragma once
#ifndef NFSMW_WIDESCREENFIX_CAMERAFOV_H
#define NFSMW_WIDESCREENFIX_CAMERAFOV_H

#include "MemoryTools.h"
#include "Resolution.h"
#include "Config.h"

namespace CameraFOV {
    // Engine Viewport IDs from eView::BuildProjectionMatrix (sub_6CF400)
    enum eViewID : uint32_t {
        VIEW_MAIN = 1,
        VIEW_SECONDARY = 2,
        VIEW_REARVIEW_MIRROR = 3,
        VIEW_REFLECTION = 4,
        VIEW_PIP = 5
    };

    inline float s_Hor3DScale = 1.0f;
    inline float s_Ver3DScale = 1.0f;
    inline float s_MirrorScale = 0.40f;

    // Direct operand overrides for eView::BuildProjectionMatrix (0x006CF400)
    inline float s_HorProjectionScale = 1.0f;    // Replaces operand at 0x006CF568 (vanilla 1.0f)
    inline float s_FovAngleMultiplier = 0.423f;   // Replaces operand at 0x006CF57A (vanilla 0.5f)
    inline float s_VerProjectionDivisor = 1.0f;  // Replaces operand at 0x006CF5DE (vanilla 1.0f)

    // Shadow frustum culling multiplier in sub_6C9570 (vanilla 1.1f at 0x006C9655)
    inline float s_ShadowDistanceMultiplier = 10.0f;

    inline const uintptr_t RET_006CF4F6 = 0x006CF4F6;
    inline const uintptr_t RET_006E465A = 0x006E465A;
    inline const uintptr_t RET_006E47EC = 0x006E47EC;

    inline void __cdecl ComputeProjectionScales(uint32_t viewID) {
        float aspect = Resolution::g_CurrentAspectRatio;
        if (aspect <= 0.0f) {
            aspect = 4.0f / 3.0f;
        }

        s_Ver3DScale = 1.0f;
        // Direct3D squash correction: preserves 1:1 pixel aspect ratio while maintaining vertical framing
        s_Hor3DScale = 1.273f / aspect;

        if (viewID == VIEW_MAIN || viewID == VIEW_REFLECTION) {
            s_HorProjectionScale = s_Hor3DScale;
            s_FovAngleMultiplier = 0.423f;
            s_VerProjectionDivisor = 1.0f;
        }
        else {
            s_HorProjectionScale = 1.0f;
            s_FovAngleMultiplier = 0.5f;
            s_VerProjectionDivisor = 1.0f;
        }
    }

    __declspec(naked) inline void Hook_BuildProjectionMatrix() {
        __asm {
            push ecx
            push edx
            push ecx
            call ComputeProjectionScales
            add esp, 4
            pop edx
            pop ecx

            mov eax, dword ptr[edi + 0x60] // eView->ViewportParams

            cmp ecx, VIEW_REARVIEW_MIRROR
            je lbl_mirror
            fld dword ptr[s_Ver3DScale]
            jmp lbl_ret

            lbl_mirror :
            fld dword ptr[s_MirrorScale]

                lbl_ret :
                jmp dword ptr[RET_006CF4F6]
        }
    }

    __declspec(naked) inline void Hook_ShadowCameraMatrix1() {
        __asm {
            fld dword ptr[s_Ver3DScale]
            jmp dword ptr[RET_006E465A]
        }
    }

    __declspec(naked) inline void Hook_ShadowCameraMatrix2() {
        __asm {
            mov dword ptr[esp + 0x14], ecx
            fld dword ptr[s_Ver3DScale]
            jmp dword ptr[RET_006E47EC]
        }
    }

    inline void UpdateFOVScale(int width, int height) {
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        if (aspect <= 0.0f) {
            aspect = 4.0f / 3.0f;
        }
        constexpr float baseAspect = 4.0f / 3.0f;

        s_Hor3DScale = 1.273f / aspect;

        // 1. eViewPlatInterface canvas virtual width (vanilla 640.0f): flt_8AE8F8
        MemoryTools::Write<float>(0x008AE8F8, 480.0f * aspect);

        // 2. FacePixelation UV scale X in arrest cutscenes (vanilla 1.0/640.0f): flt_8AFA08
        MemoryTools::Write<float>(0x008AFA08, (1.0f / 640.0f) * (baseAspect / aspect));
    }

    inline void Init(bool fixFOV = true) {
        if (!fixFOV) return;

        UpdateFOVScale(Resolution::g_CurrentWidth, Resolution::g_CurrentHeight);
        Resolution::RegisterOnResChange(UpdateFOVScale);

        // Hook eView::BuildProjectionMatrix in sub_6CF400 (5 bytes JMP + 7 NOPs)
        MemoryTools::MakeJMP(0x006CF4EA, reinterpret_cast<uintptr_t>(Hook_BuildProjectionMatrix), 7);

        // Redirect projection matrix scaling float operands
        MemoryTools::Write<float*>(0x006CF568, &s_HorProjectionScale);
        MemoryTools::Write<float*>(0x006CF57A, &s_FovAngleMultiplier);
        MemoryTools::Write<float*>(0x006CF5DE, &s_VerProjectionDivisor);

        // ShadowMap frustum culling distance in sub_6C9570 (vanilla 1.1f at 0x006C9655)
        MemoryTools::Write<float*>(0x006C9655, &s_ShadowDistanceMultiplier);

        // ShadowMap::BuildCameraMatrix hooks (sub_6E44C0)
        MemoryTools::MakeJMP(0x006E4652, reinterpret_cast<uintptr_t>(Hook_ShadowCameraMatrix1), 3);
        MemoryTools::MakeNOP(0x006E4668, 4);

        MemoryTools::MakeJMP(0x006E47E4, reinterpret_cast<uintptr_t>(Hook_ShadowCameraMatrix2), 3);
        MemoryTools::MakeNOP(0x006E47EC, 4);

        // ShadowMap camera horizontal FOV scaling
        MemoryTools::Write<float*>(0x006E46B2, &s_Hor3DScale);
        MemoryTools::Write<float*>(0x006E4832, &s_Hor3DScale);
    }
}

#endif // NFSMW_WIDESCREENFIX_CAMERAFOV_H
