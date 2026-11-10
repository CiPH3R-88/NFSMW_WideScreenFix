#pragma once
#ifndef NFSMW_WIDESCREENFIX_MOTIONBLUR_H
#define NFSMW_WIDESCREENFIX_MOTIONBLUR_H

#include "MemoryTools.h"
#include "Config.h"
#include "Resolution.h"
#include "UserSettings.h"
#include <windows.h>
#include <cstring>
#include <string>
#include <fstream>
#include <vector>

namespace Motionblur {
    // Verified from IDA: sub_6DE300 (0x006DF1D2 render bypass) and UIOptionsScreen::SetupVideo (0x00529BC7 UI injection)

    inline bool s_MotionBlurEnabled = true;
    inline int s_MotionBlurLabelHash = 0;
    inline int s_MotionBlurOnHash = 0;
    inline int s_MotionBlurOffHash = 0;
    inline unsigned int s_MotionBlurToggleVTable[15] = {};

    inline void SaveMotionBlurToIni(bool enabled) {
        if (g_Config.IniPath.empty()) return;

        std::ifstream inFile(g_Config.IniPath);
        if (!inFile.is_open()) {
            WritePrivateProfileStringA(
                "GRAPHICS",
                "MotionBlur",
                enabled ? "1                                 ; 0 = off, 1 = on"
                        : "0                                 ; 0 = off, 1 = on",
                g_Config.IniPath.string().c_str()
            );
            return;
        }

        std::vector<std::string> lines;
        std::string line;
        bool inGraphics = false;
        bool replaced = false;

        while (std::getline(inFile, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            size_t start = line.find_first_not_of(" \t");
            if (start != std::string::npos && line[start] == '[') {
                size_t end = line.find(']', start);
                if (end != std::string::npos) {
                    std::string sec = line.substr(start + 1, end - start - 1);
                    inGraphics = (_stricmp(sec.c_str(), "GRAPHICS") == 0);
                }
            } else if (inGraphics && !replaced && start != std::string::npos) {
                if (_strnicmp(&line[start], "MotionBlur", 10) == 0) {
                    char next = line[start + 10];
                    if (next == ' ' || next == '\t' || next == '=') {
                        size_t eqPos = line.find('=');
                        if (eqPos != std::string::npos) {
                            size_t semiPos = line.find(';', eqPos + 1);
                            if (semiPos != std::string::npos) {
                                size_t valStart = line.find_first_not_of(" \t", eqPos + 1);
                                size_t valEnd = line.find_last_not_of(" \t", semiPos - 1);
                                std::string beforeVal = (valStart != std::string::npos && valStart < semiPos)
                                    ? line.substr(0, valStart)
                                    : line.substr(0, eqPos + 1) + " ";
                                std::string afterVal = (valEnd != std::string::npos && valEnd + 1 < line.size())
                                    ? line.substr(valEnd + 1)
                                    : (" " + line.substr(semiPos));
                                line = beforeVal + (enabled ? "1" : "0") + afterVal;
                            } else {
                                line = line.substr(0, eqPos + 1) + (enabled ? " 1                                 ; 0 = off, 1 = on" : " 0                                 ; 0 = off, 1 = on");
                            }
                            replaced = true;
                        }
                    }
                }
            }
            lines.push_back(line);
        }
        inFile.close();

        if (replaced) {
            std::ofstream outFile(g_Config.IniPath, std::ios::trunc);
            if (outFile.is_open()) {
                for (size_t i = 0; i < lines.size(); ++i) {
                    outFile << lines[i] << "\r\n";
                }
            }
        } else {
            WritePrivateProfileStringA(
                "GRAPHICS",
                "MotionBlur",
                enabled ? "1                                 ; 0 = off, 1 = on"
                        : "0                                 ; 0 = off, 1 = on",
                g_Config.IniPath.string().c_str()
            );
        }
    }

    inline void SetMotionBlurEnabled(bool enabled, bool saveToDisk = true) {
        s_MotionBlurEnabled = enabled;

        // 1. Patch render loop branch at 0x006DF1D2 (0x74 = jz, 0xEB = jmp)
        MemoryTools::Write<uint8_t>(0x006DF1D2, enabled ? 0x74 : 0xEB);

        // 2. Set engine global g_MotionBlurEnable (0x009017DC)
        MemoryTools::Write<int>(0x009017DC, enabled ? 1 : 0);

        // 3. Update in-memory configuration
        g_Config.MotionBlur = enabled ? 1 : 0;

        if (saveToDisk) {
            // 4. Persist to NFSMW_WideScreenFix.ini while preserving inline comments
            SaveMotionBlurToIni(enabled);

            // 5. Persist to local Settings.ini if WriteSettingsToFile is active
            if (UserSettings::s_WriteSettingsToFile && !UserSettings::s_SettingsIniPath.empty()) {
                WritePrivateProfileStringA(
                    "Need for Speed Most Wanted",
                    "g_MotionBlurEnable",
                    enabled ? "1" : "0",
                    UserSettings::s_SettingsIniPath.string().c_str()
                );
            }
        }
    }

    inline void UpdateMotionBlurToggleLabel(void* widget) {
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

        // Left column (Option Name): "Motion Blur"
        auto* label = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(widget) + 0x2C);
        setLabel(label, s_MotionBlurLabelHash);

        // Right column (Option Value): "On" or "Off"
        auto* value = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(widget) + 0x30);
        setLabel(value, s_MotionBlurEnabled ? s_MotionBlurOnHash : s_MotionBlurOffHash);
    }

    inline void __fastcall MotionBlurToggleInput(void* widget, void*, int, int actionId) {
        constexpr int ACTION_DECREMENT = static_cast<int>(0x9120409E);
        constexpr int ACTION_INCREMENT = static_cast<int>(0xB5971BF1);

        if (actionId == ACTION_DECREMENT || actionId == ACTION_INCREMENT) {
            SetMotionBlurEnabled(!s_MotionBlurEnabled);
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
    }

    inline unsigned int __fastcall MotionBlurToggleLabel(void* widget) {
        UpdateMotionBlurToggleLabel(widget);
        return 0;
    }

    // Appends the custom Motion Blur toggle option after Visual Treatment in SetupVideo
    inline void __fastcall AddMotionBlurOption(void* menu, void*, void* visualTreatmentWidget, bool enabled) {
        using AddToggleOptionFn = void(__thiscall*)(void*, void*, bool);

        // 1. Add the original Visual Treatment widget
        reinterpret_cast<AddToggleOptionFn>(0x00588570)(menu, visualTreatmentWidget, enabled);

        // 2. Allocate and construct the Motion Blur FEToggleWidget
        auto gameMalloc = reinterpret_cast<void*(__cdecl*)(size_t)>(0x00652AD0);
        auto toggleCtor = reinterpret_cast<void(__thiscall*)(void*, bool)>(0x00589300);
        auto toggle = gameMalloc ? gameMalloc(0x5C) : nullptr;
        if (!toggle) return;

        toggleCtor(toggle, true);
        std::memcpy(s_MotionBlurToggleVTable, reinterpret_cast<void*>(0x0089BDB0), sizeof(s_MotionBlurToggleVTable));
        s_MotionBlurToggleVTable[1] = reinterpret_cast<unsigned int>(&MotionBlurToggleInput);
        s_MotionBlurToggleVTable[3] = reinterpret_cast<unsigned int>(&MotionBlurToggleLabel);
        *reinterpret_cast<unsigned int*>(toggle) = reinterpret_cast<unsigned int>(s_MotionBlurToggleVTable);

        // 3. Add Motion Blur toggle widget to menu
        reinterpret_cast<AddToggleOptionFn>(0x00588570)(menu, toggle, enabled);
    }

    inline void Init(bool enableMotionBlur) {
        if (g_Config.SunSetInstalled) {
            s_MotionBlurEnabled = false;
            return;
        }

        // Register display labels in the global localization map
        s_MotionBlurLabelHash = static_cast<int>(Resolution::StringHash("OPT_VO_MOTION_BLUR"));
        s_MotionBlurOnHash    = static_cast<int>(Resolution::StringHash("OPT_VO_MOTION_BLUR_ON"));
        s_MotionBlurOffHash   = static_cast<int>(Resolution::StringHash("OPT_VO_MOTION_BLUR_OFF"));

        Resolution::g_ResolutionLabels[s_MotionBlurLabelHash] = "Motion Blur";
        Resolution::g_ResolutionLabels[s_MotionBlurOnHash]    = "On";
        Resolution::g_ResolutionLabels[s_MotionBlurOffHash]   = "Off";

        // Hook SetupVideo at 0x00529BC7 to inject Motion Blur toggle after Visual Treatment
        MemoryTools::MakeCALL(0x00529BC7, reinterpret_cast<uintptr_t>(AddMotionBlurOption));

        // Set initial state according to INI configuration (no disk rewrite on boot)
        SetMotionBlurEnabled(enableMotionBlur, false);
    }
}

#endif // NFSMW_WIDESCREENFIX_MOTIONBLUR_H

