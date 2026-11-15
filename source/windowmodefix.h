#pragma once
#ifndef NFSMW_WIDESCREENFIX_WINDOWMODEFIX_H
#define NFSMW_WIDESCREENFIX_WINDOWMODEFIX_H

// ============================================================================
// Backup of In-Game Video Options Menu Toggle for Window Mode (kShowWindowModeOption)
// Kept aside as standalone backup; not connected to any active translation unit.
// ============================================================================

#include "MemoryTools.h"
#include "Config.h"
#include "Resolution.h"
#include <windows.h>
#include <cstring>
#include <string>

namespace WindowModeFix {
    inline constexpr bool kShowWindowModeOption = true;

    inline int g_WindowModeLabelHash = 0;
    inline int g_WindowModeValueHashes[6] = {};
    inline unsigned int g_WindowModeToggleVTable[15] = {};

    inline void UpdateWindowModeToggleLabel(void* widget) {
        if (!widget) return;

        auto setLabel = [](void* label, int hash) {
            if (!label) return;
            *reinterpret_cast<unsigned int*>(reinterpret_cast<uintptr_t>(label) + 0x1C) |= 0x400000;
            *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(label) + 0x60) = hash;

            // FEString::pLabelCallback global at 0x0091DD08
            auto callbackObject = *reinterpret_cast<void**>(0x0091DD08);
            if (callbackObject) {
                auto callbackVTable = *reinterpret_cast<void***>(callbackObject);
                if (callbackVTable && callbackVTable[0]) {
                    using LabelCallback = void(__thiscall*)(void*, void*);
                    reinterpret_cast<LabelCallback>(callbackVTable[0])(callbackObject, label);
                }
            }

            auto& flags = *reinterpret_cast<unsigned int*>(reinterpret_cast<uintptr_t>(label) + 0x1C);
            flags = (flags & 0xFFBFFFFD) | 0x400000;
        };

        auto* label = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(widget) + 0x2C);
        setLabel(label, g_WindowModeLabelHash);

        auto* value = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(widget) + 0x30);
        int mode = Resolution::g_WindowMode;
        if (mode < 0 || mode > 5) mode = 0;
        setLabel(value, g_WindowModeValueHashes[mode]);
    }

    inline void ApplyWindowModeRuntime() {
        *reinterpret_cast<uint32_t*>(0x00982BF0) = Resolution::g_WindowMode > 0 ? 1u : 0u;

        HWND hWnd = *reinterpret_cast<HWND*>(0x00982BF4);
        if (!hWnd) hWnd = Resolution::g_GameHWND;
        if (Resolution::g_WindowMode > 0 && hWnd) {
            Resolution::ApplyWindowStyleAndPosition(hWnd, Resolution::g_CurrentWidth, Resolution::g_CurrentHeight);
        }

        // Rebuild Direct3D device resources via eViewPlatInterface::ResetDevice
        Resolution::ResetDeviceHook();
    }

    // FEToggleWidget input handler (__thiscall virtual method replaced via __fastcall)
    inline void __fastcall WindowModeToggleInput(void* widget, void*, int, int actionId) {
        constexpr int ACTION_DECREMENT = static_cast<int>(0x9120409E);
        constexpr int ACTION_INCREMENT = static_cast<int>(0xB5971BF1);
        bool changed = false;

        if (actionId == ACTION_DECREMENT || actionId == ACTION_INCREMENT) {
            int delta = actionId == ACTION_DECREMENT ? -1 : 1;
            Resolution::g_WindowMode += delta;
            if (Resolution::g_WindowMode < 0) Resolution::g_WindowMode = 5;
            if (Resolution::g_WindowMode > 5) Resolution::g_WindowMode = 0;
            changed = true;
        }

        if (widget) {
            *reinterpret_cast<unsigned char*>(reinterpret_cast<uintptr_t>(widget) + 42) = 1;
            auto vtable = *reinterpret_cast<uintptr_t**>(widget);
            if (vtable && vtable[14]) {
                using NotifyFn = void(__thiscall*)(void*, int);
                reinterpret_cast<NotifyFn>(vtable[14])(widget, actionId);
            }
            if (vtable && vtable[3]) {
                using UpdateFn = void(__thiscall*)(void*);
                reinterpret_cast<UpdateFn>(vtable[3])(widget);
            }
        }

        if (changed) {
            ApplyWindowModeRuntime();
        }
    }

    inline unsigned int __fastcall WindowModeToggleLabel(void* widget) {
        UpdateWindowModeToggleLabel(widget);
        return 0;
    }

    // Appends the custom Window Mode FEToggleWidget via UIWidgetMenu::AddToggleOption (0x00588570)
    inline void __fastcall AddWindowModeOption(void* menu, void*, void* originalWidget, bool a4) {
        using AddToggleOptionFn = void(__thiscall*)(void*, void*, bool);
        reinterpret_cast<AddToggleOptionFn>(0x00588570)(menu, originalWidget, a4);

        auto gameMalloc = reinterpret_cast<void*(__cdecl*)(size_t)>(0x00652AD0);
        auto toggleCtor = reinterpret_cast<void(__thiscall*)(void*, bool)>(0x00589300);
        auto toggle = gameMalloc ? gameMalloc(0x5C) : nullptr;
        if (!toggle) return;

        toggleCtor(toggle, true);
        std::memcpy(g_WindowModeToggleVTable, reinterpret_cast<void*>(0x0089BDB0), sizeof(g_WindowModeToggleVTable));
        g_WindowModeToggleVTable[1] = reinterpret_cast<unsigned int>(&WindowModeToggleInput);
        g_WindowModeToggleVTable[3] = reinterpret_cast<unsigned int>(&WindowModeToggleLabel);
        *reinterpret_cast<unsigned int*>(toggle) = reinterpret_cast<unsigned int>(g_WindowModeToggleVTable);

        reinterpret_cast<AddToggleOptionFn>(0x00588570)(menu, toggle, a4);
    }

    inline void RegisterLabels() {
        g_WindowModeLabelHash = static_cast<int>(Resolution::StringHash("OPT_VO_WINDOW_MODE"));
        Resolution::g_ResolutionLabels[g_WindowModeLabelHash] = "Window Mode";
        const char* modeNames[6] = {
            "Fullscreen", "Borderless", "Border", "Resizable Border", "Borderless Fullscreen", "Borderless Fullscreen Stretched"
        };
        const char* modeHashNames[6] = {
            "OPT_VO_WINDOW_MODE_FULLSCREEN",
            "OPT_VO_WINDOW_MODE_BORDERLESS",
            "OPT_VO_WINDOW_MODE_BORDERED",
            "OPT_VO_WINDOW_MODE_RESIZABLE",
            "OPT_VO_WINDOW_MODE_SCALED",
            "OPT_VO_WINDOW_MODE_STRETCHED"
        };
        for (int i = 0; i < 6; ++i) {
            g_WindowModeValueHashes[i] = static_cast<int>(Resolution::StringHash(modeHashNames[i]));
            Resolution::g_ResolutionLabels[g_WindowModeValueHashes[i]] = modeNames[i];
        }
    }

    inline void InstallMenuHook() {
        if (*reinterpret_cast<unsigned char*>(0x00529813) == 0xE8) {
            MemoryTools::MakeCALL(0x00529813, reinterpret_cast<uintptr_t>(AddWindowModeOption));
        }
    }
}

#endif // NFSMW_WIDESCREENFIX_WINDOWMODEFIX_H

