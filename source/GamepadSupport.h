#pragma once
#ifndef NFSMW_WIDESCREENFIX_GAMEPADSUPPORT_H
#define NFSMW_WIDESCREENFIX_GAMEPADSUPPORT_H

#include <windows.h>
#include <shlobj.h>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <string>
#include <filesystem>
#include <cstdio>
#include "MemoryTools.h"
#include "Resolution.h"
#include "UserSettings.h"

namespace GamepadSupport {
    // Verified virtual addresses from IDA Pro
    constexpr uintptr_t ADDR_CREATE_RESOURCE_FILE   = 0x0065FD30; // CreateResourceFile__FPCc16ResourceFileTypeiii
    constexpr uintptr_t ADDR_RES_BEGIN_LOADING      = 0x006616F0; // BeginLoading__12ResourceFilePFi_vi
    constexpr uintptr_t ADDR_SERVICE_RES_LOADING    = 0x006626B0; // ServiceResourceLoading__Fv
    constexpr uintptr_t ADDR_FENG_SET_TEXTURE_HASH  = 0x00515B90; // FEngSetTextureHash__FP7FEImageUi
    constexpr uintptr_t ADDR_FENG_FOR_ALL_OBJECTS   = 0x005B1560; // ForAllObjects__9FEPackageR16FEObjectCallback
    constexpr uintptr_t ADDR_FENG_SERVICE_CALL      = 0x0056441F; // call site to FEngine::Update in sub_5643B0
    constexpr uintptr_t ADDR_FENG_SERVICE_TARGET    = 0x005C53C0; // Update__7FEngineiUi (cFEng::Service)
    constexpr uintptr_t ADDR_MENU_REMAP             = 0x00628A51; // sub_628940 menu navigation action remapping
    constexpr uintptr_t ADDR_CONTROLS_TEXT_CALL     = 0x006282C8; // call sub_6280C0 in sub_628230 (5 bytes: 0x6282C8 .. 0x6282CD)
    constexpr uintptr_t ADDR_CONTROLS_TEXT_TARGET   = 0x006280C0; // sub_6280C0 (FormatControllerButtonString)
    constexpr uintptr_t ADDR_START_TEXT_HASH        = 0x005A313D; // SplashScreen::SplashScreen prompt text hash operand
    constexpr uintptr_t ADDR_DEADZONE_X             = 0x007F29B2; // sub_7F28A4 Left stick X axis write [esi + 234h]
    constexpr uintptr_t ADDR_DEADZONE_Y             = 0x007F29C4; // sub_7F28A4 Left stick Y axis write [esi + 238h]
    constexpr uintptr_t ADDR_CATCH_PAD              = 0x007F2AE2; // sub_7F28A4 CatchPad button handler
    constexpr uintptr_t ADDR_BUTTONS_STATE          = 0x008F3448; // Base of joypad button mappings table (GetJoyPadMask)
    constexpr uintptr_t ADDR_GAME_STATE             = 0x00925E90; // nGameState (3 = In-game racing simulation)
    constexpr uintptr_t ADDR_B_STRING_HASH          = 0x00460BF0; // bStringHash__FPCc (EA Vatl 32-bit string hasher)
    constexpr uintptr_t ADDR_FLT_8A7A5C             = 0x008A7A5C; // Static float constant in .rdata (0.0078125f = 1.0f / 128.0f)
    constexpr uintptr_t ADDR_GAME_ACTION_TABLE      = 0x0091F7F4; // GameAction table (178 = Accept, 191 = Back, 295 = Secondary)
    constexpr uintptr_t ADDR_CONTROLLER_AVAILABLE   = 0x00628C50; // sub_628C50 (Master check for joypad availability in controls menu)
    constexpr uintptr_t ADDR_SETUP_GAMEPLAY_CALL    = 0x00529E8D; // call AddToggleOption after RacingMiniMap at the bottom of SetupGameplay (0x00529C10)
    constexpr uintptr_t ADDR_JOYPAD_PTR             = 0x0091F150; // dword_91F150 (DirectInput joypad object pointer)
    constexpr uintptr_t ADDR_SAVE_BINDS_CALL        = 0x0054603F; // UIOptionsPCController::NotificationMessage commit rebinds call (5 bytes)
    constexpr uintptr_t ADDR_SUB_51D590             = 0x0051D590; // sub_51D590 (Write rebinds from UI array to GameAction)

    // String hashes for Start Screen (verified from IDA: strings in .rdata)
    constexpr uint32_t HASH_START_PROMPT_XBOX = 0xD18D4C4C; // "Press START to begin"
    constexpr uint32_t HASH_START_PROMPT_PS   = 0xDC64C04C; // "Press START button"

    inline int   s_GamepadSupportMode = 3;   // 0 = Off, 1 = Xbox, 2 = PlayStation, 3 = Auto Detect
    inline int   s_UserForcedMode = -1;      // -1 = Auto (follows detection), 0 = Off, 1 = Xbox, 2 = PlayStation
    inline int   s_LastDetectedType = -1;
    inline float s_LeftDeadzoneFraction = 0.05f;
    inline float s_RightDeadzoneFraction = 0.05f;
    inline bool  s_LastInputWasPad = false;
    inline std::filesystem::path s_GamePath;

    // Joypad bindings structures for persistence across hotplug and restarts
    struct JoypadBindingSlot {
        int inputType;    // 0 = None, 1 = Key, 2 = Button, 3 = POV, 4 = Axis-, 5 = Axis+
        int deviceIndex;  // Controller index (0)
        int controlIndex; // Button/Axis/POV index
    };

    struct ActionJoypadBinds {
        JoypadBindingSlot slot2; // Joypad Primary (+28)
        JoypadBindingSlot slot3; // Joypad Secondary (+40)
    };

    struct SavedJoypadBindings {
        uint32_t magic;      // 0x4E465342 ('NFSB')
        uint32_t version;    // 2
        ActionJoypadBinds binds[19];
    };

    inline std::filesystem::path GetBindsFilePath() {
        if (!UserSettings::s_SettingsIniPath.empty()) {
            return UserSettings::s_SettingsIniPath.parent_path() / "NFSMW_ControllerBinds.dat";
        }
        if (UserSettings::s_UseCustomUserDir && !UserSettings::s_CustomUserDir.empty()) {
            return UserSettings::s_CustomUserDir / "NFS Most Wanted" / "NFSMW_ControllerBinds.dat";
        }
        char szDocs[MAX_PATH] = { 0 };
        SHGetFolderPathA(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, szDocs);
        std::filesystem::path docsPath = std::filesystem::path(szDocs) / "NFS Most Wanted";
        return docsPath / "NFSMW_ControllerBinds.dat";
    }

    inline bool SaveGamepadBindings() {
        SavedJoypadBindings data = {};
        data.magic = 0x4E465342;
        data.version = 2;
        for (int i = 0; i < 19; ++i) {
            uintptr_t pAction = 0x0091F418 + (i * 52);
            memcpy(&data.binds[i].slot2, reinterpret_cast<void*>(pAction + 28), sizeof(JoypadBindingSlot));
            memcpy(&data.binds[i].slot3, reinterpret_cast<void*>(pAction + 40), sizeof(JoypadBindingSlot));
        }
        std::filesystem::path path = GetBindsFilePath();
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        FILE* f = nullptr;
        if (fopen_s(&f, path.string().c_str(), "wb") == 0 && f) {
            fwrite(&data, sizeof(data), 1, f);
            fclose(f);
            return true;
        }
        return false;
    }

    inline bool LoadGamepadBindings() {
        std::filesystem::path path = GetBindsFilePath();
        FILE* f = nullptr;
        if (fopen_s(&f, path.string().c_str(), "rb") != 0 || !f) {
            return false;
        }
        SavedJoypadBindings data = {};
        size_t readBytes = fread(&data, 1, sizeof(data), f);
        fclose(f);
        if (readBytes != sizeof(data) || data.magic != 0x4E465342 || data.version != 2) {
            return false;
        }
        for (int i = 0; i < 19; ++i) {
            uintptr_t pAction = 0x0091F418 + (i * 52);
            memcpy(reinterpret_cast<void*>(pAction + 28), &data.binds[i].slot2, sizeof(JoypadBindingSlot));
            memcpy(reinterpret_cast<void*>(pAction + 40), &data.binds[i].slot3, sizeof(JoypadBindingSlot));
        }
        return true;
    }

    inline int __fastcall Hooked_sub_51D590(void* pThis, void* /*edx*/) {
        using OrigFn = int(__thiscall*)(void*);
        auto origFn = reinterpret_cast<OrigFn>(ADDR_SUB_51D590);
        int res = origFn(pThis);
        SaveGamepadBindings();
        return res;
    }

    // DirectInput 8 Device Instance structure (for hardware VID/PID extraction without dinput8.lib)
    struct DI_DeviceInstanceA {
        DWORD dwSize;
        GUID  guidInstance;
        GUID  guidProduct;
        DWORD dwDevType;
        char  tszInstanceName[260];
        char  tszProductName[260];
        GUID  guidFFDriver;
        WORD  wUsagePage;
        WORD  wUsage;
    };

    // Check if the physical DirectInput device is currently attached to the system
    inline bool IsJoypadAttached(uintptr_t pJoypad, bool force = false) {
        if (!pJoypad || pJoypad == static_cast<uintptr_t>(-4)) return false;

        static DWORD s_LastCheckTick = 0;
        static bool s_CachedAttached = true;
        DWORD now = GetTickCount();
        if (!force && (now - s_LastCheckTick < 250)) {
            return s_CachedAttached;
        }
        s_LastCheckTick = now;

        void* pDevice = *reinterpret_cast<void**>(pJoypad + 1128);
        if (!pDevice) {
            s_CachedAttached = false;
            return false;
        }

        void** devVtbl = *reinterpret_cast<void***>(pDevice);
        if (!devVtbl || !devVtbl[15]) {
            s_CachedAttached = false;
            return false;
        }

        DI_DeviceInstanceA di = {};
        di.dwSize = sizeof(DI_DeviceInstanceA);
        using GetDeviceInfoFn = HRESULT(__stdcall*)(void*, DI_DeviceInstanceA*);
        auto getDevInfo = reinterpret_cast<GetDeviceInfoFn>(devVtbl[15]);
        if (getDevInfo(pDevice, &di) < 0) {
            s_CachedAttached = false;
            return false;
        }

        uintptr_t* pDInputSys = *reinterpret_cast<uintptr_t**>(0x009BB410);
        if (pDInputSys) {
            void* pDInput = reinterpret_cast<void*>(pDInputSys[45]);
            if (pDInput) {
                void** dinputVtbl = *reinterpret_cast<void***>(pDInput);
                if (dinputVtbl && dinputVtbl[5]) {
                    using GetDeviceStatusFn = HRESULT(__stdcall*)(void*, const GUID*);
                    auto getDeviceStatus = reinterpret_cast<GetDeviceStatusFn>(dinputVtbl[5]);
                    HRESULT hrStatus = getDeviceStatus(pDInput, &di.guidInstance);
                    if (hrStatus != 0) { // DI_OK is 0; DIERR_NOTATTACHED is 0x800401E3
                        s_CachedAttached = false;
                        return false;
                    }
                }
            }
        }

        if (devVtbl[25]) { // Poll
            using PollFn = HRESULT(__stdcall*)(void*);
            auto poll = reinterpret_cast<PollFn>(devVtbl[25]);
            HRESULT hrPoll = poll(pDevice);
            if (hrPoll == static_cast<HRESULT>(0x800401E3L) /* DIERR_UNPLUGGED */) {
                s_CachedAttached = false;
                return false;
            }
        }

        s_CachedAttached = true;
        return true;
    }

    inline void HandleControllerDisconnected() {
        *reinterpret_cast<uintptr_t*>(ADDR_JOYPAD_PTR) = 0;
        *reinterpret_cast<int*>(0x009203B8) = 0;
        s_LastDetectedType = 0;
        s_UserForcedMode = -1; // Reset manual override back to auto (OFF)
        s_LastInputWasPad = false; // Switch icons back to PC / Keyboard
        MemoryTools::Write<uint32_t>(ADDR_START_TEXT_HASH, HASH_START_PROMPT_XBOX);
    }

    // Dynamically scan for newly connected gamepads if none was detected at engine boot
    inline void TryHotplugController(bool force = false) {
        uintptr_t currentPad = *reinterpret_cast<uintptr_t*>(ADDR_JOYPAD_PTR);
        if (currentPad != 0 && currentPad != static_cast<uintptr_t>(-4)) {
            if (IsJoypadAttached(currentPad, force)) {
                static bool s_InitialBindsSaved = false;
                if (!s_InitialBindsSaved) {
                    bool hasBinds = false;
                    for (int i = 0; i < 19; ++i) {
                        uintptr_t pAction = 0x0091F418 + (i * 52);
                        int inputType = *reinterpret_cast<int*>(pAction + 28);
                        if (inputType > 0) {
                            hasBinds = true;
                            break;
                        }
                    }
                    if (hasBinds) {
                        s_InitialBindsSaved = true;
                        std::filesystem::path path = GetBindsFilePath();
                        if (!std::filesystem::exists(path)) {
                            SaveGamepadBindings();
                        }
                    }
                }
                return;
            }
            HandleControllerDisconnected();
        }

        static DWORD s_LastScanTick = 0;
        DWORD now = GetTickCount();
        if (!force && (now - s_LastScanTick < 1000)) {
            return; // Rate limit automatic scanning to once per second
        }
        s_LastScanTick = now;

        uintptr_t* pDInputSys = *reinterpret_cast<uintptr_t**>(0x009BB410);
        if (!pDInputSys) return;

        void* pDInput = reinterpret_cast<void*>(pDInputSys[45]);
        if (!pDInput) return;

        void** vtbl = *reinterpret_cast<void***>(pDInput);
        if (!vtbl || !vtbl[4]) return;

        // Reset joypad count so newly enumerated pad assigns to slot 0 (dword_91F150[0])
        *reinterpret_cast<int*>(0x009203B8) = 0;

        using EnumDevicesFn = HRESULT(__stdcall*)(void*, DWORD, int(__stdcall*)(int, int), void*, DWORD);
        auto enumDevices = reinterpret_cast<EnumDevicesFn>(vtbl[4]);
        enumDevices(
            pDInput,
            4, // DI8DEVCLASS_GAMECTRL
            reinterpret_cast<int(__stdcall*)(int, int)>(0x007F1A6B),
            pDInputSys,
            1  // DIEDFL_ATTACHEDONLY
        );

        uintptr_t newPad = *reinterpret_cast<uintptr_t*>(ADDR_JOYPAD_PTR);
        if (newPad && newPad != static_cast<uintptr_t>(-4)) {
            // 1. DirectInput vs XInput caps check
            reinterpret_cast<void(__cdecl*)()>(0x006282E0)();

            // Backup active joypad bindings from GameAction before sub_63F850 overwrites them
            ActionJoypadBinds preBinds[19];
            bool hadActiveBinds = false;
            for (int i = 0; i < 19; ++i) {
                uintptr_t pAction = 0x0091F418 + (i * 52);
                memcpy(&preBinds[i].slot2, reinterpret_cast<void*>(pAction + 28), sizeof(JoypadBindingSlot));
                memcpy(&preBinds[i].slot3, reinterpret_cast<void*>(pAction + 40), sizeof(JoypadBindingSlot));
                if (preBinds[i].slot2.inputType > 0 || preBinds[i].slot3.inputType > 0) {
                    hadActiveBinds = true;
                }
            }

            // 2. Joypad acquisition and default GameAction bindings
            auto getIO = reinterpret_cast<char*(__cdecl*)()>(0x00634370);
            if (getIO) {
                char* pIO = getIO();
                void* pInputMgr = pIO ? *reinterpret_cast<void**>(pIO + 8) : nullptr;
                if (pInputMgr) {
                    using InitJoypadFn = int(__thiscall*)(void*);
                    reinterpret_cast<InitJoypadFn>(0x0063F850)(pInputMgr);
                }
            }

            // Restore custom bindings (prefer persistent file, fallback to pre-hotplug memory backup)
            if (!LoadGamepadBindings()) {
                if (hadActiveBinds) {
                    for (int i = 0; i < 19; ++i) {
                        uintptr_t pAction = 0x0091F418 + (i * 52);
                        memcpy(reinterpret_cast<void*>(pAction + 28), &preBinds[i].slot2, sizeof(JoypadBindingSlot));
                        memcpy(reinterpret_cast<void*>(pAction + 40), &preBinds[i].slot3, sizeof(JoypadBindingSlot));
                    }
                    SaveGamepadBindings();
                }
            }

            // 3. Maintain console menu navigation bindings
            *reinterpret_cast<uint32_t*>(0x0091FABC) = 0;
            *reinterpret_cast<uint32_t*>(0x0091FAF0) = 1;
            *reinterpret_cast<uint32_t*>(0x0091FC90) = 3;
        }
    }

    // Auto-detect whether a connected controller is Xbox or PlayStation layout
    inline int DetectConnectedControllerType() {
        uintptr_t pJoypad = *reinterpret_cast<uintptr_t*>(ADDR_JOYPAD_PTR);
        if (!pJoypad || pJoypad == static_cast<uintptr_t>(-4)) {
            TryHotplugController(false);
            pJoypad = *reinterpret_cast<uintptr_t*>(ADDR_JOYPAD_PTR);
            if (!pJoypad || pJoypad == static_cast<uintptr_t>(-4)) {
                return 0; // No controller connected -> OFF
            }
        }

        // Verify that the registered joypad is still physically attached to the system
        if (!IsJoypadAttached(pJoypad)) {
            HandleControllerDisconnected();
            return 0; // Unplugged -> OFF
        }

        auto isPlayStationMatch = [](const char* str) -> bool {
            if (!str || !*str) return false;
            char lower[260] = { 0 };
            for (size_t i = 0; i < 259 && str[i]; ++i) {
                lower[i] = static_cast<char>(tolower(static_cast<unsigned char>(str[i])));
            }
            return strstr(lower, "dualshock") != nullptr ||
                   strstr(lower, "dualsense")  != nullptr ||
                   strstr(lower, "playstation")!= nullptr ||
                   strstr(lower, "sony")       != nullptr ||
                   strstr(lower, "wireless controller") != nullptr ||
                   strstr(lower, "ds3")        != nullptr ||
                   strstr(lower, "ds4")        != nullptr ||
                   strstr(lower, "ds5")        != nullptr ||
                   strstr(lower, "sixaxis")    != nullptr ||
                   strstr(lower, "ps3")        != nullptr ||
                   strstr(lower, "ps4")        != nullptr ||
                   strstr(lower, "ps5")        != nullptr;
        };

        // 1. Try reading hardware Vendor ID via DirectInput device interface at offset +1128
        void* pDevice = *reinterpret_cast<void**>(pJoypad + 1128);
        if (pDevice) {
            void** vtbl = *reinterpret_cast<void***>(pDevice);
            if (vtbl && vtbl[15]) {
                DI_DeviceInstanceA di = {};
                di.dwSize = sizeof(DI_DeviceInstanceA);
                using GetDeviceInfoFn = HRESULT(__stdcall*)(void*, DI_DeviceInstanceA*);
                auto getDevInfo = reinterpret_cast<GetDeviceInfoFn>(vtbl[15]);
                if (getDevInfo(pDevice, &di) >= 0) {
                    uint16_t vid = static_cast<uint16_t>(di.guidProduct.Data1 & 0xFFFF);
                    if (vid == 0x054C) { // Sony Interactive Entertainment
                        return 2; // PLAYSTATION
                    }
                    if (vid == 0x045E) { // Microsoft
                        return 1; // XBOX
                    }
                    if (isPlayStationMatch(di.tszProductName) || isPlayStationMatch(di.tszInstanceName)) {
                        return 2; // PLAYSTATION
                    }
                } else {
                    // Device communication failed (e.g. unplugged)
                    *reinterpret_cast<uintptr_t*>(ADDR_JOYPAD_PTR) = 0;
                    *reinterpret_cast<int*>(0x009203B8) = 0;
                    return 0;
                }
            }
        }

        // 2. Intelligent pattern matching on product and instance names
        const char* prodName = reinterpret_cast<const char*>(pJoypad + 276); // tszProductName
        const char* instName = reinterpret_cast<const char*>(pJoypad + 16);  // tszInstanceName

        if (isPlayStationMatch(prodName) || isPlayStationMatch(instName)) {
            return 2; // PLAYSTATION
        }

        // Default connected gamepads to Xbox (PC standard)
        return 1; // XBOX
    }

    inline int GetEffectiveMode() {
        if (s_GamepadSupportMode != 3) {
            return s_GamepadSupportMode; // Forced in .ini (0, 1, or 2)
        }

        int detected = DetectConnectedControllerType();
        if (s_LastDetectedType != -1 && detected != s_LastDetectedType) {
            // Physical controller connection changed -> reset manual override so auto-detection takes over
            s_UserForcedMode = -1;
        }
        s_LastDetectedType = detected;

        if (s_UserForcedMode != -1) {
            return s_UserForcedMode; // User manually chose in Gameplay Options
        }
        return detected; // Auto-detect
    }

    // Checks whether gamepad input is currently permitted:
    // Requires effective mode > 0 (not OFF) AND the selected mode matches the physically connected hardware type.
    inline bool IsControllerAllowed() {
        int effMode = GetEffectiveMode();
        if (effMode <= 0) {
            return false;
        }
        int physicalType = DetectConnectedControllerType();
        return physicalType == effMode;
    }

    // Master hook for sub_628C50: Suppresses controller bind tab in controls menu when controller is disabled, mismatched, or unplugged
    inline BOOL __cdecl Hooked_sub_628C50() {
        TryHotplugController(false);
        uintptr_t currentPad = *reinterpret_cast<uintptr_t*>(ADDR_JOYPAD_PTR);
        if (!currentPad || currentPad == static_cast<uintptr_t>(-4) || !IsJoypadAttached(currentPad, true)) {
            HandleControllerDisconnected();
            return FALSE;
        }
        if (!IsControllerAllowed()) {
            return FALSE;
        }
        return TRUE;
    }

    // Native EA Vatl/Bernstein string hasher (0x00460BF0)
    inline uint32_t HashString(const char* str) {
        if (!str || !*str) return 0;
        auto fn = reinterpret_cast<uint32_t(__cdecl*)(const char*)>(ADDR_B_STRING_HASH);
        return fn(str);
    }

    struct TextureMapping {
        const char* pcName;
        const char* xboxName;
        const char* psName;
        uint32_t pcHash;
        uint32_t xboxHash;
        uint32_t psHash;
    };

    inline TextureMapping s_Mappings[] = {
        { "L1",                      "XBOX_L1",                   "PS_L1",                   0, 0, 0 },
        { "PC_KEY_Q",                "XBOX_KEY_Q",                "PS_KEY_Q",                0, 0, 0 },
        { "PC_GREATERTHAN_LESSTHAN", "XBOX_GREATERTHAN_LESSTHAN", "PS_GREATERTHAN_LESSTHAN", 0, 0, 0 },
        { "R1",                      "XBOX_R1",                   "PS_R1",                   0, 0, 0 },
        { "PC_KEY_ENTER",            "XBOX_KEY_ENTER",            "PS_KEY_ENTER",            0, 0, 0 },
        { "PC_KEY_ESC_FRENCH",       "XBOX_KEY_ESC_FRENCH",       "PS_KEY_ESC_FRENCH",       0, 0, 0 },
        { "PC_KEY_ESC",              "XBOX_KEY_ESC",              "PS_KEY_ESC",              0, 0, 0 },
        { "PC_KEY_1",                "XBOX_KEY_1",                "PS_KEY_1",                0, 0, 0 },
        { "PC_KEY_2",                "XBOX_KEY_2",                "PS_KEY_2",                0, 0, 0 },
        { "PC_KEY_3",                "XBOX_KEY_3",                "PS_KEY_3",                0, 0, 0 },
        { "PC_KEY_4",                "XBOX_KEY_4",                "PS_KEY_4",                0, 0, 0 },
        { "PC_ARROW_KEYS_UP_DOWN",   "XBOX_ARROW_KEYS_UP_DOWN",   "PS_ARROW_KEYS_UP_DOWN",   0, 0, 0 },
        { "PC_LOAD",                 "XBOX_LOAD",                 "PS_LOAD",                 0, 0, 0 },
        { "PC_ARROW_KEYS",           "XBOX_ARROW_KEYS",           "PS_ARROW_KEYS",           0, 0, 0 },
    };

    // Verified from IDA: Engine resource streaming pipeline (LoadGlobalAChunks at 0x00662B30)
    // CreateResourceFile (0x0065FD30) -> ResourceFile::BeginLoading (0x006616F0) -> ServiceResourceLoading (0x006626B0)
    inline void EnsureCustomTexturesLoaded() {
        static bool s_Loaded = false;
        if (s_Loaded) return;
        s_Loaded = true;

        const char* targetRelPath = "GLOBAL\\NFSMW_WideScreenFix.bin";
        if (std::filesystem::exists(s_GamePath / "GLOBAL" / "NFSMW_WideScreenFix.bin")) {
            targetRelPath = "GLOBAL\\NFSMW_WideScreenFix.bin";
        } else if (std::filesystem::exists(s_GamePath / "scripts" / "NFSMW_WideScreenFix.bin")) {
            targetRelPath = "scripts\\NFSMW_WideScreenFix.bin";
        } else if (std::filesystem::exists(s_GamePath / "NFSMW_WideScreenFix.bin")) {
            targetRelPath = "NFSMW_WideScreenFix.bin";
        } else if (std::filesystem::exists(s_GamePath / "GLOBAL" / "NFSMW_WideScreenFix.tpk")) {
            targetRelPath = "GLOBAL\\NFSMW_WideScreenFix.tpk";
        } else if (std::filesystem::exists(s_GamePath / "scripts" / "NFSMW_WideScreenFix.tpk")) {
            targetRelPath = "scripts\\NFSMW_WideScreenFix.tpk";
        }

        auto createResFn = reinterpret_cast<void*(__cdecl*)(const char*, int, int, int, int)>(ADDR_CREATE_RESOURCE_FILE);
        auto beginLoadFn = reinterpret_cast<void(__thiscall*)(void*, void*, void*)>(ADDR_RES_BEGIN_LOADING);
        auto serviceResFn = reinterpret_cast<void(__cdecl*)()>(ADDR_SERVICE_RES_LOADING);

        void* rFile = createResFn(targetRelPath, 0, 0, 0, 0);
        if (rFile) {
            beginLoadFn(rFile, nullptr, nullptr);
            serviceResFn();
        }
    }

    // Verified from IDA: FEObject::Type at offset +0x18 (FE_Image = 1), Flags at +0x1C, Handle at +0x24 (36)
    inline void ProcessFEObject(void* pObj) {
        if (!pObj) return;

        // FEObject::Type is at offset 0x18 (1 = FE_Image)
        uint32_t objType = *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(pObj) + 0x18);
        if (objType != 1) return;

        // FEImage::Handle (texture hash) is at offset 0x24
        uint32_t currentHandle = *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(pObj) + 0x24);

        for (const auto& m : s_Mappings) {
            if (currentHandle == m.pcHash || currentHandle == m.xboxHash || currentHandle == m.psHash) {
                uint32_t newHash = m.pcHash;
                int effMode = GetEffectiveMode();
                if (s_LastInputWasPad && effMode > 0) {
                    if (effMode == 1) newHash = m.xboxHash;
                    else if (effMode == 2) newHash = m.psHash;
                }

                if (newHash && newHash != currentHandle) {
                    // Direct handle & dirty flag update (Flags |= 0x2400000)
                    *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(pObj) + 0x24) = newHash;
                    uint32_t flags = *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(pObj) + 0x1C);
                    *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(pObj) + 0x1C) = flags | 0x02400000;
                }
                break;
            }
        }
    }

    // Verified from IDA: FEPackage::ForAllObjects (0x005B1560) invokes callback via [eax+04h]
    struct FEObjectCallbackStruct {
        void* Destructor = nullptr;
        void* Function = nullptr;
    };

    inline char __stdcall Callback_FEObjectVisitor(void* pObj) {
        ProcessFEObject(pObj);
        return 1;
    }

    // Verified from IDA: cFEng::mInstance loaded packages linked list head at +0xE0, next at +0x04 (cFEng::MakeLoadedPackagesDirty at 0x00525A00)
    inline void UpdateActivePackages(void* pFEng) {
        if (!pFEng) return;
        EnsureCustomTexturesLoaded();

        FEObjectCallbackStruct callback = { nullptr, reinterpret_cast<void*>(&Callback_FEObjectVisitor) };
        void* cbpointer = &callback;
        auto forAllFn = reinterpret_cast<char(__fastcall*)(void*, void*, void*)>(ADDR_FENG_FOR_ALL_OBJECTS);

        // Linked list of active loaded packages is at pFEng + 0xE0
        uintptr_t current = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(pFEng) + 0xE0);
        while (current) {
            forAllFn(reinterpret_cast<void*>(current), nullptr, &cbpointer);
            uintptr_t next = *reinterpret_cast<uintptr_t*>(current + 4);
            if (next == current || next == 0) break;
            current = next;
        }

        // Maintain console menu navigation bindings (A=Accept, B=Back, Y=Options) without touching sub_628940
        *reinterpret_cast<uint32_t*>(0x0091FABC) = 0;
        *reinterpret_cast<uint32_t*>(0x0091FAF0) = 1;
        *reinterpret_cast<uint32_t*>(0x0091FC90) = 3;
    }

    // Verified from IDA: Hook site at 0x0056441F in sub_5643B0 (calls Update__7FEngineiUi at 0x005C53C0)
    inline void __fastcall Hooked_cFEng_Service(void* pThis, void* /*edx*/, uint32_t deltaTicks, uint32_t lock) {
        UpdateActivePackages(pThis);
        auto origFn = reinterpret_cast<void(__thiscall*)(void*, uint32_t, uint32_t)>(ADDR_FENG_SERVICE_TARGET);
        origFn(pThis, deltaTicks, lock);
    }

    // Verified from IDA: In-game Controls Menu Text Translation at sub_628230 exit (0x006282E3)
    struct ControlLabel {
        const char* key;
        const char* xbox;
        const char* ps;
    };

    static const ControlLabel s_Labels[] = {
        { "Button 1",    "A",                      "Cross" },
        { "Button 2",    "B",                      "Circle" },
        { "Button 3",    "X",                      "Square" },
        { "Button 4",    "Y",                      "Triangle" },
        { "Button 5",    "LB",                     "L1" },
        { "Button 6",    "RB",                     "R1" },
        { "Button 7",    "Back",                   "Select" },
        { "Button 8",    "Start",                  "Start" },
        { "Button 9",    "LSB",                    "L3" },
        { "Button 10",   "RSB",                    "R3" },
        { " 1",          "A",                      "Cross" },
        { " 2",          "B",                      "Circle" },
        { " 3",          "X",                      "Square" },
        { " 4",          "Y",                      "Triangle" },
        { " 5",          "LB",                     "L1" },
        { " 6",          "RB",                     "R1" },
        { " 7",          "Back",                   "Select" },
        { " 8",          "Start",                  "Start" },
        { " 9",          "LSB",                    "L3" },
        { " 10",         "RSB",                    "R3" },
        { "Up",          "D-pad Up",               "D-pad Up" },
        { "Down",        "D-pad Down",             "D-pad Down" },
        { "Left",        "D-pad Left",             "D-pad Left" },
        { "Right",       "D-pad Right",            "D-pad Right" },
        { "X Rotation",  "Right stick",            "Right stick" },
        { "Y Rotation",  "Right stick",            "Right stick" },
        { "X Axis",      "Left stick",             "Left stick" },
        { "Y Axis",      "Left stick",             "Left stick" },
        { "Z Axis",      "LT / RT",                "L2 / R2" },
        { "Hat Switch",  "D-pad",                  "D-pad" }
    };

    inline const char* ResolveDpadDirection(int a4_binding) {
        if (!a4_binding || static_cast<uintptr_t>(a4_binding) < 0x10000) return "D-pad";
        auto* p = reinterpret_cast<const int*>(a4_binding);
        // Verified from game engine memory:
        // p[0] = 3 (POV Hat), p[1] = 0 (Device 0), p[2] = Direction:
        // 0 = Up, 1 = Right, 2 = Down, 3 = Left
        int dir = p[2];
        if (dir == 0) return "D-pad Up";
        if (dir == 1) return "D-pad Right";
        if (dir == 2) return "D-pad Down";
        if (dir == 3) return "D-pad Left";

        return "D-pad";
    }

    inline const char* ResolveTriggerLabel(int a4_binding) {
        int effMode = GetEffectiveMode();
        if (!a4_binding || static_cast<uintptr_t>(a4_binding) < 0x10000) return (effMode == 2) ? "L2 / R2" : "LT / RT";
        auto* p = reinterpret_cast<const int*>(a4_binding);
        // Verified from game engine memory:
        // p[2] = 2 (Z-axis), p[0] = deflection direction:
        // 4 = RT / R2 (Negative deflection)
        // 5 = LT / L2 (Positive deflection)
        if (p[0] == 4) return (effMode == 2) ? "R2" : "RT";
        if (p[0] == 5) return (effMode == 2) ? "L2" : "LT";

        return (effMode == 2) ? "L2 / R2" : "LT / RT";
    }

    inline void __cdecl FormatControlsString(char* buffer, int a4_binding) {
        if (!buffer || !*buffer) return;
        int effMode = GetEffectiveMode();
        if (effMode <= 0) return;

        std::string s(buffer);

        // Check if this is a Hat Switch / D-pad binding
        if (s.find("Hat Switch") != std::string::npos || s == "D-pad") {
            const char* dpadLabel = ResolveDpadDirection(a4_binding);
            strcpy_s(buffer, 64, dpadLabel);
            return;
        }

        // Check if this is a Z Axis / Trigger binding
        if (s.find("Z Axis") != std::string::npos || s == "LT / RT" || s == "L2 / R2") {
            const char* triggerLabel = ResolveTriggerLabel(a4_binding);
            strcpy_s(buffer, 64, triggerLabel);
            return;
        }

        for (const auto& item : s_Labels) {
            size_t pos = s.find(item.key);
            if (pos != std::string::npos) {
                // Prevent "Button 1" or " 1" from falsely matching "Button 10" or " 10"
                if ((strcmp(item.key, "Button 1") == 0 || strcmp(item.key, " 1") == 0) &&
                    pos + strlen(item.key) < s.length() && s[pos + strlen(item.key)] == '0') {
                    continue;
                }
                const char* replacement = (effMode == 2) ? item.ps : item.xbox;
                strcpy_s(buffer, 64, replacement);
                return;
            }
        }
    }

    inline void* __cdecl Hooked_sub_6280C0(char* outBuffer, size_t bufSize, int a3, int a4) {
        auto origFn = reinterpret_cast<void*(__cdecl*)(char*, size_t, int, int)>(ADDR_CONTROLS_TEXT_TARGET);
        void* res = origFn(outBuffer, bufSize, a3, a4);
        if (outBuffer) {
            FormatControlsString(outBuffer, a4);
        }
        return res;
    }

    // Verified from IDA: Left stick proportional deadzone hooks for axis writes in sub_7F28A4
    // Left stick X axis store at 0x007F29B2 (mov [esi + 234h], eax)
    // Left stick Y axis store at 0x007F29C4 (mov [esi + 238h], edx)
    constexpr uintptr_t RET_ADDR_STICK_X = 0x007F29B8;
    constexpr uintptr_t RET_ADDR_STICK_Y = 0x007F29CA;

    inline uint32_t __cdecl FilterAxisX(uint32_t rawVal) {
        double d = (static_cast<double>(rawVal) / 65535.0) - 0.5;
        if (s_LeftDeadzoneFraction > 0.0f && std::abs(d) <= s_LeftDeadzoneFraction) d = 0.0;
        return static_cast<uint32_t>((d + 0.5) * 65535.0);
    }

    inline uint32_t __cdecl FilterAxisY(uint32_t rawVal) {
        double d = (static_cast<double>(rawVal) / 65535.0) - 0.5;
        if (s_LeftDeadzoneFraction > 0.0f && std::abs(d) <= s_LeftDeadzoneFraction) d = 0.0;
        return static_cast<uint32_t>((d + 0.5) * 65535.0);
    }

    inline void __declspec(naked) Hook_StickAxisX() {
        __asm {
            push edx
            push ecx
            push eax
            call FilterAxisX
            add esp, 4
            mov dword ptr [esi + 0x234], eax
            pop ecx
            pop edx
            jmp dword ptr [RET_ADDR_STICK_X]
        }
    }

    inline void __declspec(naked) Hook_StickAxisY() {
        __asm {
            push edx
            push ecx
            push edx
            call FilterAxisY
            add esp, 4
            mov dword ptr [esi + 0x238], eax
            pop ecx
            pop edx
            jmp dword ptr [RET_ADDR_STICK_Y]
        }
    }

    // Verified from IDA: Main input action polling call in sub_6349B0 at 0x00634B00 (calls sub_628940)
    // Applies a 5% deadzone filter to eliminate resting half-integer offset (+0.5f / 0.000015f) on analog triggers/axes,
    // which previously caused digital actions (e.g. Look Behind, Pull Camera Back) mapped to RT to be permanently held down.
    constexpr uintptr_t ADDR_ACTION_POLL_CALL   = 0x00634B00;
    constexpr uintptr_t ADDR_ACTION_POLL_TARGET = 0x00628940;

    inline double __stdcall Hooked_ActionPoll(int actionIndex, int slot) {
        // GameAction array at 0x0091F418: each action is 52 bytes, each slot is 12 bytes
        // Slot binding: [0] = deviceIndex, [1] = deviceType (1 = Keyboard, 2..9 = Joypad)
        uintptr_t pBinding = 0x0091F418 + (actionIndex * 52) + (slot * 12);
        int deviceType = *reinterpret_cast<int*>(pBinding + 4);

        if (deviceType > 1) { // Joypad binding
            if (!IsControllerAllowed()) {
                return 0.0; // Controller disabled or mismatched: zero out all gamepad actions
            }
            auto origFn = reinterpret_cast<double(__stdcall*)(int, int)>(ADDR_ACTION_POLL_TARGET);
            double val = origFn(actionIndex, slot);
            if (val < 0.05) {
                return 0.0;
            }
            if (val >= 0.1) {
                s_LastInputWasPad = true;
            }
            return val;
        }

        // Keyboard binding (deviceType == 1) or other: always allow through
        auto origFn = reinterpret_cast<double(__stdcall*)(int, int)>(ADDR_ACTION_POLL_TARGET);
        return origFn(actionIndex, slot);
    }

    // Verified from IDA: CatchPad at 0x007F2AE2 in sub_7F28A4
    // Reads controller buttons at regs.esi + 0x234, maps:
    // Select -> ButtonsState + 0xCC (0x008F3514 / World Map / Look Back)
    // X      -> ButtonsState + 0x13C (0x008F3584 / Speedbreaker)
    // Start  -> ButtonsState + 0xB0 (0x008F34F8 / Pause Menu)
    // LSClick + RSClick simultaneously in-game (nGameState == 3 at 0x00925E90) -> Q key (Reset Car)
    constexpr uintptr_t RET_ADDR_CATCH_PAD = 0x007F2AE9;

    struct PadKeyStates {
        int32_t LSAxis1;
        int32_t LSAxis2;
        int32_t LTRT;
        int32_t A;
        int32_t B;
        int32_t X;
        int32_t Y;
        int32_t LB;
        int32_t RB;
        int32_t Select;
        int32_t Start;
        int32_t LSClick;
        int32_t RSClick;
    };

    inline void __cdecl ProcessCatchPad(uintptr_t esi_val) {
        if (!esi_val || esi_val == 1) return;
        if (!IsControllerAllowed()) {
            return;
        }
        auto* pad = reinterpret_cast<PadKeyStates*>(esi_val + 0x234);
        if (!pad) return;

        if (pad->A || pad->B || pad->X || pad->Y || pad->LB || pad->RB ||
            pad->Select || pad->Start || pad->LSClick || pad->RSClick) {
            s_LastInputWasPad = true;
        }

        if (pad->Start) {
            *reinterpret_cast<int32_t*>(ADDR_BUTTONS_STATE + 0xB0) = 1;
        }

        if (pad->LSClick && pad->RSClick) {
            int32_t gameState = *reinterpret_cast<int32_t*>(ADDR_GAME_STATE);
            if (gameState == 3) {
                BYTE vkQ = static_cast<BYTE>(VkKeyScanA('Q'));
                keybd_event(vkQ, 0, 0, 0);
                keybd_event(vkQ, 0, KEYEVENTF_KEYUP, 0);
            }
        }
    }

    inline void __declspec(naked) Hook_CatchPad() {
        __asm {
            mov dword ptr [ebp - 0x20], 1 // original instruction
            push eax
            push ecx
            push edx
            push esi
            call ProcessCatchPad
            add esp, 4
            pop edx
            pop ecx
            pop eax
            jmp dword ptr [RET_ADDR_CATCH_PAD]
        }
    }

    // Called when keyboard key or mouse movement is detected
    inline void OnKeyboardOrMouseInput() {
        s_LastInputWasPad = false;
    }

    // Gameplay Options Toggle Widget (FEToggleWidget) for Controller Selection
    inline int s_ControllerLabelHash = 0;
    inline int s_ControllerOffHash = 0;
    inline int s_ControllerXboxHash = 0;
    inline int s_ControllerPSHash = 0;
    inline unsigned int s_ControllerToggleVTable[15] = {};

    inline void RegisterLabels() {
        s_ControllerLabelHash = static_cast<int>(Resolution::StringHash("OPT_GP_CONTROLLER"));
        Resolution::g_ResolutionLabels[s_ControllerLabelHash] = "Controller";

        s_ControllerOffHash = static_cast<int>(Resolution::StringHash("OPT_GP_CONTROLLER_OFF"));
        Resolution::g_ResolutionLabels[s_ControllerOffHash] = "Off";

        s_ControllerXboxHash = static_cast<int>(Resolution::StringHash("OPT_GP_CONTROLLER_XBOX"));
        Resolution::g_ResolutionLabels[s_ControllerXboxHash] = "Xbox";

        s_ControllerPSHash = static_cast<int>(Resolution::StringHash("OPT_GP_CONTROLLER_PLAYSTATION"));
        Resolution::g_ResolutionLabels[s_ControllerPSHash] = "PlayStation";
    }

    inline void UpdateControllerToggleLabel(void* widget) {
        if (!widget) return;

        TryHotplugController(false);
        uintptr_t currentPad = *reinterpret_cast<uintptr_t*>(ADDR_JOYPAD_PTR);
        if (currentPad && currentPad != static_cast<uintptr_t>(-4) && !IsJoypadAttached(currentPad, true)) {
            HandleControllerDisconnected();
        }

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

        // Left column (Option Name): "Controller"
        auto* label = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(widget) + 0x2C);
        setLabel(label, s_ControllerLabelHash);

        // Right column (Option Value): "Off", "Xbox", "PlayStation" based on GetEffectiveMode()
        auto* value = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(widget) + 0x30);
        int mode = GetEffectiveMode();
        int valHash = s_ControllerOffHash;
        if (mode == 1) valHash = s_ControllerXboxHash;
        else if (mode == 2) valHash = s_ControllerPSHash;
        setLabel(value, valHash);
    }

    inline void __fastcall ControllerToggleInput(void* widget, void*, int, int actionId) {
        constexpr int ACTION_DECREMENT = static_cast<int>(0x9120409E);
        constexpr int ACTION_INCREMENT = static_cast<int>(0xB5971BF1);

        if (actionId == ACTION_DECREMENT || actionId == ACTION_INCREMENT) {
            TryHotplugController(true);
            uintptr_t currentPad = *reinterpret_cast<uintptr_t*>(ADDR_JOYPAD_PTR);
            if (currentPad && currentPad != static_cast<uintptr_t>(-4) && !IsJoypadAttached(currentPad, true)) {
                HandleControllerDisconnected();
            }
            int current = GetEffectiveMode();
            int delta = (actionId == ACTION_DECREMENT) ? -1 : 1;
            int next = current + delta;
            if (next < 0) next = 2;
            if (next > 2) next = 0;
            s_UserForcedMode = next;

            if (next == 2) {
                MemoryTools::Write<uint32_t>(ADDR_START_TEXT_HASH, HASH_START_PROMPT_PS);
            } else {
                MemoryTools::Write<uint32_t>(ADDR_START_TEXT_HASH, HASH_START_PROMPT_XBOX);
            }

            if (next == 0) {
                s_LastInputWasPad = false;
            } else {
                s_LastInputWasPad = true;
            }
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

    inline unsigned int __fastcall ControllerToggleLabel(void* widget) {
        UpdateControllerToggleLabel(widget);
        return 0;
    }

    // Appends the custom Controller option at the bottom of SetupGameplay (0x00529C10) via UIWidgetMenu::AddToggleOption (0x00588570)
    inline void __fastcall AddControllerOption(void* menu, void*, void* originalWidget, bool enabled) {
        using AddToggleOptionFn = void(__thiscall*)(void*, void*, bool);

        // 1. Add original widget (RacingMiniMap)
        reinterpret_cast<AddToggleOptionFn>(0x00588570)(menu, originalWidget, enabled);

        // 2. Only add Controller option if GamepadSupport = 3 (Auto detect)
        if (s_GamepadSupportMode != 3) return;

        // 3. Allocate and construct FEToggleWidget (0x5C bytes)
        auto gameMalloc = reinterpret_cast<void*(__cdecl*)(size_t)>(0x00652AD0);
        auto toggleCtor = reinterpret_cast<void(__thiscall*)(void*, bool)>(0x00589300);
        auto toggle = gameMalloc ? gameMalloc(0x5C) : nullptr;
        if (!toggle) return;

        toggleCtor(toggle, true);
        std::memcpy(s_ControllerToggleVTable, reinterpret_cast<void*>(0x0089BDB0), sizeof(s_ControllerToggleVTable));
        s_ControllerToggleVTable[1] = reinterpret_cast<unsigned int>(&ControllerToggleInput);
        s_ControllerToggleVTable[3] = reinterpret_cast<unsigned int>(&ControllerToggleLabel);
        *reinterpret_cast<unsigned int*>(toggle) = reinterpret_cast<unsigned int>(s_ControllerToggleVTable);

        // 4. Add Controller toggle widget to menu
        reinterpret_cast<AddToggleOptionFn>(0x00588570)(menu, toggle, enabled);
    }

    // Initialize all gamepad support features
    inline void Init(int mode, float leftDeadzone, float rightDeadzone, const std::filesystem::path& gamePath) {
        s_GamepadSupportMode = mode;
        s_GamePath = gamePath;

        RegisterLabels();

        Resolution::s_OnInputActivity = &OnKeyboardOrMouseInput;

        // Master hook for controls menu joypad availability check (sub_628C50)
        // Suppresses controller bind tab when mode is OFF (effective mode == 0)
        MemoryTools::MakeJMP(ADDR_CONTROLLER_AVAILABLE, reinterpret_cast<uintptr_t>(Hooked_sub_628C50));

        // Inject Controller option in Gameplay Options menu only if GamepadSupport == 3 (Auto Detect)
        if (s_GamepadSupportMode == 3) {
            MemoryTools::MakeCALL(ADDR_SETUP_GAMEPLAY_CALL, reinterpret_cast<uintptr_t>(AddControllerOption));
        }

        if (s_GamepadSupportMode == 0) {
            // Mode 0: strictly Keyboard only. Controller check already suppressed via Hooked_sub_628C50.
            return;
        }

        s_LastInputWasPad = true;

        if (leftDeadzone > 0.0f) {
            s_LeftDeadzoneFraction = leftDeadzone / 200.0f;
        } else {
            s_LeftDeadzoneFraction = 0.0f;
        }

        if (rightDeadzone > 0.0f) {
            s_RightDeadzoneFraction = rightDeadzone / 200.0f;
        } else {
            s_RightDeadzoneFraction = 0.0f;
        }

        // Left stick deadzone hooks (if configured)
        if (s_LeftDeadzoneFraction > 0.0f) {
            MemoryTools::MakeJMP(ADDR_DEADZONE_X, reinterpret_cast<uintptr_t>(Hook_StickAxisX), 1);
            MemoryTools::MakeJMP(ADDR_DEADZONE_Y, reinterpret_cast<uintptr_t>(Hook_StickAxisY), 1);
        }

        // Compute texture hashes once
        for (auto& m : s_Mappings) {
            m.pcHash   = HashString(m.pcName);
            m.xboxHash = HashString(m.xboxName);
            m.psHash   = HashString(m.psName);
        }

        // 1. Hook FEng Service call to swap UI button icons
        MemoryTools::MakeCALL(ADDR_FENG_SERVICE_CALL, reinterpret_cast<uintptr_t>(Hooked_cFEng_Service));

        // 2. Set console menu navigation defaults (A=Accept, B=Back, Y=Options)
        MemoryTools::Write<uint32_t>(0x0091FABC, 0);
        MemoryTools::Write<uint32_t>(0x0091FAF0, 1);
        MemoryTools::Write<uint32_t>(0x0091FC90, 3);

        // 3. Hook in-game controls menu button text formatting (5-byte call at 0x006282C8)
        MemoryTools::MakeCALL(ADDR_CONTROLS_TEXT_CALL, reinterpret_cast<uintptr_t>(Hooked_sub_6280C0));

        // 4. Hook CatchPad for Start button pause and LS+RS car reset
        MemoryTools::MakeJMP(ADDR_CATCH_PAD, reinterpret_cast<uintptr_t>(Hook_CatchPad), 2);

        // 5. Start Screen prompt text
        if (GetEffectiveMode() == 2) {
            MemoryTools::Write<uint32_t>(ADDR_START_TEXT_HASH, HASH_START_PROMPT_PS);
        } else {
            MemoryTools::Write<uint32_t>(ADDR_START_TEXT_HASH, HASH_START_PROMPT_XBOX);
        }

        // 6. Hook action polling in sub_6349B0 to apply resting deadzone on triggers/axes
        MemoryTools::MakeCALL(ADDR_ACTION_POLL_CALL, reinterpret_cast<uintptr_t>(Hooked_ActionPoll));

        // 7. Hook Controls Menu rebind save (UIOptionsPCController::NotificationMessage -> sub_51D590)
        MemoryTools::MakeCALL(ADDR_SAVE_BINDS_CALL, reinterpret_cast<uintptr_t>(Hooked_sub_51D590));
    }
}

#endif // NFSMW_WIDESCREENFIX_GAMEPADSUPPORT_H
