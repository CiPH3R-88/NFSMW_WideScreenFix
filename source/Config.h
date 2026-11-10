#pragma once
#ifndef NFSMW_WIDESCREENFIX_CONFIG_H
#define NFSMW_WIDESCREENFIX_CONFIG_H

#include <windows.h>
#include <string>
#include <filesystem>

struct Settings {
    // DISPLAY
    int ResX = 0;
    int ResY = 0;
    int WindowMode = 0; // 0 = Fullscreen | 1 = Borderless | 2 = Border | 3 = Resizable Border | 4 = Borderless Fullscreen | 5 = Borderless Fullscreen Stretched

    // GRAPHICS
    int MotionBlur = 1; // 0 = Off, 1 = On

    // HUD
    int FixHUD = 1;
    int WideScreenHUD = 0;
    int WS_MWSPLASHBACK = 0;
    bool X360StuffInstalled = false;
    bool SunSetInstalled = false;

    // MOVIES
    int WideScreenMovie = 1; // 0 = Off, 1 = Zoom/Fill 16:9, 2 = Fit with Pillarboxes

    // SHADOW
    int ShadowsRes = 2048;
    int ShadowsFix = 1;
    int ImproveShadowLOD = 1;
    int AutoScaleShadowsRes = 1;
    int ShadowMapTextureFormat = 0;
    int ForceSharpShadows = 0;

    // NOSTRAIL
    int NosTrail = 1;
    float NOSTrailLength = 0.0f;
    float NOSTrailPositionScalar = 0.0f;

    // MISC
    int SkipIntro = 0;
    int SimRate = 1; // 0 = Off | 1 = Dynamic (Matches monitor refresh rate, Default) | 2 = Fixed 200Hz | > 2 = Custom Hz
    int WriteSettingsToFile = 0;
    int WinCursor = 0; // 0 = Off (default, block Windows cursor), 1 = On (allow Windows cursor)
    std::string CustomUserFilesDirectoryInGameDir = "";

    std::filesystem::path IniPath;
    std::filesystem::path GamePath;

    void Load() {
        char exePath[MAX_PATH] = { 0 };
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        GamePath = std::filesystem::path(exePath).parent_path();
        IniPath = GamePath / "scripts" / "NFSMW_WideScreenFix.ini";
        if (!std::filesystem::exists(IniPath)) {
            IniPath = GamePath / "NFSMW_WideScreenFix.ini";
        }

        std::string iniStr = IniPath.string();

        ResX = GetPrivateProfileIntA("DISPLAY", "ResX", 0, iniStr.c_str());
        ResY = GetPrivateProfileIntA("DISPLAY", "ResY", 0, iniStr.c_str());
        WindowMode = GetPrivateProfileIntA("DISPLAY", "WindowMode", -1, iniStr.c_str());
        if (WindowMode == -1) {
            WindowMode = GetPrivateProfileIntA("MISC", "WindowedMode", -1, iniStr.c_str());
            if (WindowMode == -1) {
                WindowMode = GetPrivateProfileIntA("DISPLAY", "Windowed", 0, iniStr.c_str());
            }
        }

        MotionBlur = GetPrivateProfileIntA("GRAPHICS", "MotionBlur", 1, iniStr.c_str());
        FixHUD = GetPrivateProfileIntA("HUD", "FixHUD", 1, iniStr.c_str());
        WideScreenHUD = GetPrivateProfileIntA("HUD", "WideScreenHUD", 0, iniStr.c_str());
        WS_MWSPLASHBACK = GetPrivateProfileIntA("HUD", "WS_MWSPLASHBACK", 0, iniStr.c_str());

        WideScreenMovie = GetPrivateProfileIntA("MOVIES", "WideScreenMovie", 1, iniStr.c_str());

        // SHADOW
        ShadowsRes = GetPrivateProfileIntA("SHADOW", "ShadowsRes", 2048, iniStr.c_str());
        ShadowsFix = GetPrivateProfileIntA("SHADOW", "ShadowsFix", 1, iniStr.c_str());
        ImproveShadowLOD = GetPrivateProfileIntA("SHADOW", "ImproveShadowLOD", 1, iniStr.c_str());
        AutoScaleShadowsRes = GetPrivateProfileIntA("SHADOW", "AutoScaleShadowsRes", 1, iniStr.c_str());
        ShadowMapTextureFormat = GetPrivateProfileIntA("SHADOW", "ShadowMapTextureFormat", 0, iniStr.c_str());
        ForceSharpShadows = GetPrivateProfileIntA("SHADOW", "ForceSharpShadows", 0, iniStr.c_str());

        // NOSTRAIL (support both [NOSTRAIL] and [NOS TRAIL])
        NosTrail = GetPrivateProfileIntA("NOSTRAIL", "NosTrail", -1, iniStr.c_str());
        if (NosTrail == -1) NosTrail = GetPrivateProfileIntA("NOS TRAIL", "NosTrail", 1, iniStr.c_str());

        char nosLenBuf[64] = { 0 };
        GetPrivateProfileStringA("NOSTRAIL", "NOSTrailLength", "", nosLenBuf, sizeof(nosLenBuf), iniStr.c_str());
        if (!nosLenBuf[0]) GetPrivateProfileStringA("NOS TRAIL", "NOSTrailLength", "0", nosLenBuf, sizeof(nosLenBuf), iniStr.c_str());
        NOSTrailLength = static_cast<float>(atof(nosLenBuf));
        if (NOSTrailLength < 0.0f) NOSTrailLength = 0.0f;

        char nosPosBuf[64] = { 0 };
        GetPrivateProfileStringA("NOSTRAIL", "NOSTrailPositionScalar", "", nosPosBuf, sizeof(nosPosBuf), iniStr.c_str());
        if (!nosPosBuf[0]) GetPrivateProfileStringA("NOS TRAIL", "NOSTrailPositionScalar", "0.0", nosPosBuf, sizeof(nosPosBuf), iniStr.c_str());
        NOSTrailPositionScalar = static_cast<float>(atof(nosPosBuf));
        if (NOSTrailPositionScalar < 0.0f) NOSTrailPositionScalar = 0.0f;

        // Toru's X360Stuff applies HUD aspect correction, widescreen placement,
        // and entirely replaces the shadow map rendering pipeline.
        X360StuffInstalled = std::filesystem::exists(GamePath / "scripts" / "X360Stuff.asi");

        // Sunset mod provides its own in-game Motion Blur video option and custom particle shaders.
        SunSetInstalled = std::filesystem::exists(GamePath / "scripts" / "SunSet.asi");

        SkipIntro = GetPrivateProfileIntA("MISC", "SkipIntro", 0, iniStr.c_str());
        SimRate = GetPrivateProfileIntA("MISC", "SimRate", 1, iniStr.c_str());
        WriteSettingsToFile = GetPrivateProfileIntA("MISC", "WriteSettingsToFile", 0, iniStr.c_str());
        WinCursor = GetPrivateProfileIntA("MISC", "WinCursor", 0, iniStr.c_str());
        if (!WinCursor) {
            WinCursor = GetPrivateProfileIntA("MISC", "Win Cursor", 0, iniStr.c_str());
        }

        char userDirBuf[MAX_PATH] = { 0 };
        GetPrivateProfileStringA("MISC", "CustomUserFilesDirectoryInGameDir", "0", userDirBuf, MAX_PATH, iniStr.c_str());
        std::string userDir = userDirBuf;
        auto semi = userDir.find(';');
        if (semi != std::string::npos) userDir = userDir.substr(0, semi);
        auto hash = userDir.find('#');
        if (hash != std::string::npos) userDir = userDir.substr(0, hash);
        while (!userDir.empty() && (userDir.back() == ' ' || userDir.back() == '\t' || userDir.back() == '\r' || userDir.back() == '\n')) {
            userDir.pop_back();
        }
        while (!userDir.empty() && (userDir.front() == ' ' || userDir.front() == '\t')) {
            userDir.erase(userDir.begin());
        }
        if (userDir == "0") {
            userDir.clear();
        }
        CustomUserFilesDirectoryInGameDir = userDir;
    }
};

inline Settings g_Config;

#endif // NFSMW_WIDESCREENFIX_CONFIG_H
