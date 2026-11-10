#include <windows.h>

#include "Config.h"
#include "MemoryTools.h"
#include "Resolution.h"
#include "CameraFOV.h"
#include "HudFix.h"
#include "MovieFix.h"
#include "SkipIntro.h"
#include "UserSettings.h"
#include "Motionblur.h"
#include "SimRate.h"
#include "Shadowfix.h"
#include "NOSTrailfix.h"
#include "HighSpecAudio.h"

static void InitializeMod() {
    static bool s_Initialized = false;
    if (s_Initialized) return;
    s_Initialized = true;

    // 1. Load configuration from NFSMW_WideScreenFix.ini
    g_Config.Load();

    // 2. Resolution System (Menu integration, seamless runtime switching, display mode enumeration)
    Resolution::Init(g_Config.ResX, g_Config.ResY, g_Config.WindowMode);

    // 3. User & Savegame Settings (CustomUserDir & WriteSettingsToFile)
    if (!g_Config.CustomUserFilesDirectoryInGameDir.empty() && g_Config.CustomUserFilesDirectoryInGameDir != "0") {
        UserSettings::InitCustomUserFiles(g_Config.GamePath, g_Config.CustomUserFilesDirectoryInGameDir);
    }
    if (g_Config.WriteSettingsToFile) {
        UserSettings::InitRegistryRedirect(g_Config.GamePath, true);
    }

    // 4. Skip Intro Logos/FMVs
    if (g_Config.SkipIntro) {
        SkipIntro::Init();
    }

    // 5. FMV / Movie Widescreen & Full Color Range. X360Stuff owns movie scaling when installed.
    MovieFix::Init(g_Config.WideScreenMovie, g_Config.X360StuffInstalled);

    // 6. 3D Camera Field of View (Hor+ Projection)
    CameraFOV::Init();

    // 7. Motion Blur. Sunset mod owns its own Video Settings option when installed.
    if (!g_Config.SunSetInstalled) {
        Motionblur::Init(g_Config.MotionBlur != 0);
    } else {
        Motionblur::s_MotionBlurEnabled = false;
    }

    // 6. 2D HUD. X360Stuff owns the same HUD transform when installed.
    if (!g_Config.X360StuffInstalled) {
        HudFix::Init(g_Config.FixHUD != 0, g_Config.WideScreenHUD != 0);
    }

    // Independent of X360Stuff detection: this only changes the loading
    // splash package name when explicitly enabled.
    HudFix::InitWideScreenSplash(g_Config.WS_MWSPLASHBACK != 0);

    // 7. Windows Cursor Toggle (0 = Default in-game cursor, blocks Windows cursor | 1 = Windows OS cursor)
    HudFix::InitWinCursor(g_Config.WinCursor);

    // 8. High-Precision SimRate (200 Hz Decoupled Physics)
    if (g_Config.SimRate != 0) {
        SimRate::Init(true, g_Config.SimRate);
    }

    // 9. Dynamic Shadows System (ShadowsRes, ShadowsFix, LOD, Formats)
    ShadowFix::Init(
        g_Config.ShadowsRes,
        g_Config.ShadowsFix,
        g_Config.ImproveShadowLOD,
        g_Config.AutoScaleShadowsRes,
        g_Config.ShadowMapTextureFormat,
        g_Config.ForceSharpShadows
    );

    // 10. NOS Trail (Enables/disables trail, optional custom length and position offset)
    NOSTrailFix::Init(
        g_Config.NosTrail != 0,
        g_Config.NOSTrailLength,
        g_Config.NOSTrailPositionScalar
    );

    // 11. High-Spec Audio (Forces 44,100 Hz CD-quality audio by default)
    HighSpecAudio::Init();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);

        // Safety check: only initialize when attached to speed.exe
        char moduleName[MAX_PATH] = { 0 };
        GetModuleFileNameA(NULL, moduleName, MAX_PATH);
        if (strstr(moduleName, "speed.exe") || strstr(moduleName, "SPEED.EXE")) {
            InitializeMod();
        }
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        SimRate::Shutdown();
    }
    return TRUE;
}
