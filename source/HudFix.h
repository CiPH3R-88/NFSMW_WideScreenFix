#pragma once
#ifndef NFSMW_WIDESCREENFIX_HUDFIX_H
#define NFSMW_WIDESCREENFIX_HUDFIX_H

#include "MemoryTools.h"
#include "Resolution.h"
#include <cmath>
#include <unordered_map>

namespace HudFix {
    inline float s_HudMatrixScaleX = 1.0f;
    inline float s_HudScaleX = 0.003125f; // flt_8AF9A4 (vanilla: 2.0f / 640.0f)
    inline float s_FEHudPosX = 320.0f;    // flt_894B40 (vanilla: 320.0f)

    // Verified from IDA: SplashScreen::SplashScreen (0x005A3030) and package string at 0x008A0114
    inline void InitWideScreenSplash(bool enabled) {
        if (!enabled) return;

        // 1. NOP the jz short loc_5A30A2 at 0x005A3080 in SplashScreen::SplashScreen
        MemoryTools::MakeNOP(0x005A3080, 2);

        // 2. Overwrite the package name in .rdata at 0x008A0114
        constexpr char kwsSplashName[] = "WS_MW_LS_Splash.fng";
        MemoryTools::WriteBytes(0x008A0114, kwsSplashName, sizeof(kwsSplashName));

        // 3. Set prompt text hash to "CLICK to continue" (PC) at 0x005A313D
        MemoryTools::Write<uint32_t>(0x005A313D, 0xC4DF3FF2);

        // 4. Force GetSplashScreenPackageName() to return widescreen splash
        MemoryTools::Write<uint8_t>(0x005705EC, 0xEB);
    }

    inline bool s_WideScreenHUD = false;
    inline float s_WideScreenHUDOffset = 0.0f;

    namespace WideScreenHUD {
        // Native FEng Engine API functions in speed.exe
        using FEngFindObjectFn = void*(__cdecl*)(const char* packageName, unsigned int objectHash);
        using FEngGetCenterFn  = void(__cdecl*)(void* object, float* outX, float* outY);
        using FEngSetCenterFn  = void(__cdecl*)(void* object, float x, float y);
        using QueuePackageMessageFn = void(__fastcall*)(void* instance, void* edx, int messageHash, const char* packageName, int a4);
        using SetWideScreenModeFn   = char(__thiscall*)(void* instance);

        inline const auto FEngFindObject = reinterpret_cast<FEngFindObjectFn>(0x00524850);
        inline const auto FEngGetCenter  = reinterpret_cast<FEngGetCenterFn>(0x00524EE0);
        inline const auto FEngSetCenter  = reinterpret_cast<FEngSetCenterFn>(0x00525050);

        inline QueuePackageMessageFn s_OriginalQueuePackageMessage = nullptr;
        inline SetWideScreenModeFn   s_OriginalSetWideScreenMode   = nullptr;

        inline std::unordered_map<void*, std::pair<float, float>> s_OriginalCenters;

        struct MinimapOriginalPositions {
            float localX;
            float childX[5];
        };
        inline std::unordered_map<void*, MinimapOriginalPositions> s_MinimapOriginalPositions;
        inline bool s_Installed = false;

        // Moves FEng HUD groups to the widescreen screen borders
        inline void ApplyGroupOffset(const char* packageName) {
            if (!packageName) return;

            constexpr unsigned int LEFT_GROUP  = 0x1603009E; // Minimap group
            constexpr unsigned int RIGHT_GROUP = 0x5D0101F1; // Tachometer / Speedometer group

            void* left  = FEngFindObject(packageName, LEFT_GROUP);
            void* right = FEngFindObject(packageName, RIGHT_GROUP);

            auto move = [](void* object, float delta) {
                if (!object) return;
                auto it = s_OriginalCenters.find(object);
                if (it == s_OriginalCenters.end()) {
                    float x = 0.0f, y = 0.0f;
                    FEngGetCenter(object, &x, &y);
                    it = s_OriginalCenters.emplace(object, std::make_pair(x, y)).first;
                }
                FEngSetCenter(object, it->second.first + delta, it->second.second);
            };

            move(left, -s_WideScreenHUDOffset);
            move(right, s_WideScreenHUDOffset);
        }

        inline void __fastcall QueuePackageMessageHook(void* instance, void* edx,
            int messageHash, const char* packageName, int a4) {
            if (s_OriginalQueuePackageMessage) {
                s_OriginalQueuePackageMessage(instance, edx, messageHash, packageName, a4);
            }
            ApplyGroupOffset(packageName);
        }

        inline char __fastcall SetWideScreenModeHook(void* instance, void* edx) {
            char result = s_OriginalSetWideScreenMode ? s_OriginalSetWideScreenMode(instance) : 0;
            return result;
        }

        // Native Minimap::AdjustForWidescreen (0x005678A0) dynamic displacement
        inline int __fastcall AdjustMinimapForWidescreen(
            void* minimap, void* edx, char isWidescreen) {
            if (!minimap) return 0;

            const float offset = isWidescreen ? -s_WideScreenHUDOffset : s_WideScreenHUDOffset;

            if (!s_MinimapOriginalPositions.empty() &&
                s_MinimapOriginalPositions.begin()->first != minimap) {
                s_MinimapOriginalPositions.clear();
            }

            MemoryTools::Write<float>(0x0091CF04, isWidescreen ? -s_WideScreenHUDOffset : 0.0f);
            MemoryTools::Write<float>(0x0091CF0C, isWidescreen ? -0.9375f : 0.9375f);

            constexpr int childOffsets[5] = { 60, 64, 68, 72, 144 };
            auto it = s_MinimapOriginalPositions.find(minimap);
            if (it == s_MinimapOriginalPositions.end()) {
                MinimapOriginalPositions original{};
                original.localX = *reinterpret_cast<float*>(
                    reinterpret_cast<uint8_t*>(minimap) + 184);

                for (int i = 0; i < 5; ++i) {
                    void* child = *reinterpret_cast<void**>(
                        reinterpret_cast<uint8_t*>(minimap) + childOffsets[i]);
                    if (!child) return 0;
                    void* data = *reinterpret_cast<void**>(
                        reinterpret_cast<uint8_t*>(child) + 44);
                    if (!data) return 0;
                    original.childX[i] = *reinterpret_cast<float*>(
                        reinterpret_cast<uint8_t*>(data) + 28);
                }
                it = s_MinimapOriginalPositions.emplace(minimap, original).first;
            }

            const auto& original = it->second;
            MemoryTools::Write<float>(
                reinterpret_cast<uintptr_t>(minimap) + 184,
                original.localX + offset);

            for (int i = 0; i < 5; ++i) {
                void* child = *reinterpret_cast<void**>(
                    reinterpret_cast<uint8_t*>(minimap) + childOffsets[i]);
                if (!child) continue;
                void* data = *reinterpret_cast<void**>(
                    reinterpret_cast<uint8_t*>(child) + 44);
                if (!data) continue;
                MemoryTools::Write<float>(
                    reinterpret_cast<uintptr_t>(data) + 28,
                    original.childX[i] + offset);
            }

            void* lastChild = *reinterpret_cast<void**>(
                reinterpret_cast<uint8_t*>(minimap) + 144);
            if (lastChild) {
                void* data = *reinterpret_cast<void**>(
                    reinterpret_cast<uint8_t*>(lastChild) + 44);
                if (data) {
                    const uint32_t value = *reinterpret_cast<uint32_t*>(
                        reinterpret_cast<uint8_t*>(minimap) + 188);
                    MemoryTools::Write<uint32_t>(
                        reinterpret_cast<uintptr_t>(data) + 32, value);
                }
            }

            return 0;
        }

        inline void UpdateOffset(int width, int height) {
            const float aspect = static_cast<float>(width) / static_cast<float>(height);
            s_WideScreenHUDOffset = (480.0f / 2.0f) * aspect - (640.0f / 2.0f);
        }

        inline void Init() {
            if (s_Installed) return;

            // Hook cFEng::QueuePackageMessage (0x00516C90) at call site
            s_OriginalQueuePackageMessage = reinterpret_cast<QueuePackageMessageFn>(0x00516C90);
            MemoryTools::MakeCALL(0x005696F1, reinterpret_cast<uintptr_t>(QueuePackageMessageHook));
            MemoryTools::MakeCALL(0x00569712, reinterpret_cast<uintptr_t>(QueuePackageMessageHook));

            // FEngHud::SetWideScreenMode (0x005696C0)
            s_OriginalSetWideScreenMode = reinterpret_cast<SetWideScreenModeFn>(0x005696C0);

            // 1. Force FEngHud::SetWideScreenMode video option read (0x005696CB: mov al, 1; nop)
            const uint8_t forceWideScreen[] = { 0xB0, 0x01, 0x90 };
            MemoryTools::WriteBytes(0x005696CB, forceWideScreen, sizeof(forceWideScreen));

            // 2. Force FEngHud::DetermineHudFeatures comparison (0x0057CB82: cmp dl, 1)
            const uint8_t forceHudFeatureFlag[] = { 0x80, 0xFA, 0x01 };
            MemoryTools::WriteBytes(0x0057CB82, forceHudFeatureFlag, sizeof(forceHudFeatureFlag));

            // 3. Force Chyron::Start EA Trax song banner read (0x0058D883: mov al, 1; nop)
            MemoryTools::WriteBytes(0x0058D883, forceWideScreen, sizeof(forceWideScreen));

            // Hook FEngHud::Update caller at 0x0058CAD2
            MemoryTools::MakeCALL(0x0058CAD2, reinterpret_cast<uintptr_t>(SetWideScreenModeHook));

            // Hook Minimap::AdjustForWidescreen callers
            MemoryTools::MakeCALL(0x00569702, reinterpret_cast<uintptr_t>(AdjustMinimapForWidescreen));
            MemoryTools::MakeCALL(0x00569723, reinterpret_cast<uintptr_t>(AdjustMinimapForWidescreen));

            s_Installed = true;
        }
    }

    inline void UpdateWideScreenHUD(int width, int height) {
        if (!s_WideScreenHUD) return;
        const float aspect = static_cast<float>(width) / static_cast<float>(height);
        s_WideScreenHUDOffset = (480.0f / 2.0f) * aspect - (640.0f / 2.0f);
    }

    using RenderPolyFn = int(__cdecl*)(void*, void*, int);
    inline RenderPolyFn s_OriginalMirrorRender = nullptr;

    inline int __cdecl MirrorRenderHook(void* poly, void* textureInfo, int flags) {
        if (poly) {
            float* v = reinterpret_cast<float*>(poly);
            // In vanilla 4:3, rearview mirror is centered at 320.0f with half-width 130.0f (190.0f to 450.0f).
            // With widescreen projection scale (s_HudScaleX) active, centering the quad around s_FEHudPosX
            // places the mirror in the top-center with correct 1:1 aspect ratio.
            constexpr float halfWidth = 130.0f;
            v[4]  = s_FEHudPosX - halfWidth;
            v[8]  = s_FEHudPosX - halfWidth;
            v[0]  = s_FEHudPosX + halfWidth;
            v[12] = s_FEHudPosX + halfWidth;
        }
        return s_OriginalMirrorRender ? s_OriginalMirrorRender(poly, textureInfo, flags) : 0;
    }

    inline void UpdateHudScale(int width, int height) {
        const float aspect = static_cast<float>(width) / static_cast<float>(height);

        // Global orthographic projection scale across 480p virtual height:
        // s_HudScaleX = (1.0f / width * (height / 480.0f)) * 2.0f = 2.0f / (480.0f * aspect)
        s_HudScaleX = (1.0f / static_cast<float>(width) * (static_cast<float>(height) / 480.0f)) * 2.0f;
        // Global HUD center in virtual coordinates: (480.0f * aspect) / 2.0f
        s_FEHudPosX = 640.0f / (640.0f * s_HudScaleX);

        // 1. Global 2D projection scale in eViewPlatInterface::Render (0x006D1260): flt_8AF9A4
        MemoryTools::Write<float>(0x008AF9A4, s_HudScaleX);

        // 2. Global hit-test center used by button hovers (sub_530A60), carousel arrows (sub_587190),
        // sliders (sub_554DF0), and GetMouseInfo (0x00516870): flt_894B40
        MemoryTools::Write<float>(0x00894B40, s_FEHudPosX);

        // 3. Matrix translation _41 sites: replace hardcoded 320.0f with s_FEHudPosX
        MemoryTools::Write<float>(0x0056FECD + 7, s_FEHudPosX); // 0x0056FED4: Cursor matrix (esp + 0x19C)
        MemoryTools::Write<float>(0x00584EEF + 7, s_FEHudPosX); // 0x00584EF6: Base HUD matrix (esp + 0x90)
        MemoryTools::Write<float>(0x00599E79 + 7, s_FEHudPosX); // 0x00599E80: RenderMovie (esp + 0x84)
        MemoryTools::Write<float>(0x0059A120 + 7, s_FEHudPosX); // 0x0059A127: RenderMultiImage (esp + 0xC4)
        MemoryTools::Write<float>(0x0059A5AB + 7, s_FEHudPosX); // 0x0059A5B2: RenderImage (esp + 0x94)
        MemoryTools::Write<float>(0x0059A83E + 7, s_FEHudPosX); // 0x0059A845: RenderImage2 (esp + 0x94)
        MemoryTools::Write<float>(0x005A44C8 + 4, s_FEHudPosX); // 0x005A44CC: RenderString (esp + 0x64)
    }

    inline void Init(bool fixHUD, bool wideScreenHUD) {
        if (!fixHUD && !wideScreenHUD) return;

        s_WideScreenHUD = wideScreenHUD;

        if (fixHUD) {
            constexpr uintptr_t kMirrorRenderCallSite = 0x006E7199;
            if (!s_OriginalMirrorRender) {
                s_OriginalMirrorRender = reinterpret_cast<RenderPolyFn>(
                    MemoryTools::GetCallTarget(kMirrorRenderCallSite));
                MemoryTools::MakeCALL(kMirrorRenderCallSite,
                    reinterpret_cast<uintptr_t>(MirrorRenderHook));
            }

            UpdateHudScale(Resolution::g_CurrentWidth, Resolution::g_CurrentHeight);
            Resolution::RegisterOnResChange(UpdateHudScale);
        }

        if (wideScreenHUD) {
            UpdateWideScreenHUD(Resolution::g_CurrentWidth, Resolution::g_CurrentHeight);
            WideScreenHUD::UpdateOffset(Resolution::g_CurrentWidth, Resolution::g_CurrentHeight);
            WideScreenHUD::Init();
            Resolution::RegisterOnResChange(UpdateWideScreenHUD);
            Resolution::RegisterOnResChange(WideScreenHUD::UpdateOffset);
        }
    }
}

#endif // NFSMW_WIDESCREENFIX_HUDFIX_H
