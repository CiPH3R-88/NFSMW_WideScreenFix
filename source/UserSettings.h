#pragma once
#ifndef NFSMW_WIDESCREENFIX_USERSETTINGS_H
#define NFSMW_WIDESCREENFIX_USERSETTINGS_H

#include "MemoryTools.h"
#include "Resolution.h"
#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <string>
#include <map>

namespace UserSettings {
    // ------------------------------------------------------------------------
    // Feature 1: CustomUserFilesDirectoryInGameDir (Savegame & Profile Redirect)
    // ------------------------------------------------------------------------
    inline std::filesystem::path s_CustomUserDir;
    inline bool s_UseCustomUserDir = false;

    using PFN_SHGetFolderPathA = HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR);
    inline PFN_SHGetFolderPathA s_OrigSHGetFolderPathA = nullptr;

    inline HRESULT WINAPI Hook_SHGetFolderPathA(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath) {
        if (s_UseCustomUserDir) {
            int folder = csidl & ~CSIDL_FLAG_CREATE;
            if (folder == CSIDL_PERSONAL) {
                std::error_code ec;
                std::filesystem::create_directories(s_CustomUserDir, ec);
                std::string pathStr = s_CustomUserDir.string();
                if (pszPath) {
                    std::memcpy(pszPath, pathStr.c_str(), pathStr.size() + 1);
                    return S_OK;
                }
            }
        }

        if (s_OrigSHGetFolderPathA) {
            return s_OrigSHGetFolderPathA(hwnd, csidl, hToken, dwFlags, pszPath);
        }
        return SHGetFolderPathA(hwnd, csidl, hToken, dwFlags, pszPath);
    }

    inline void InitCustomUserFiles(const std::filesystem::path& gamePath, const std::string& customDirName) {
        if (customDirName.empty() || customDirName == "0") return;

        s_CustomUserDir = gamePath / customDirName;
        s_UseCustomUserDir = true;
        std::error_code ec;
        std::filesystem::create_directories(s_CustomUserDir, ec);

        // Patch speed.exe fixed IAT address at 0x008902EC (SHFOLDER.dll!SHGetFolderPathA)
        PROC* pDirectIAT = reinterpret_cast<PROC*>(0x008902EC);
        s_OrigSHGetFolderPathA = reinterpret_cast<PFN_SHGetFolderPathA>(*pDirectIAT);
        DWORD oldProtect;
        MemoryTools::Unprotect(reinterpret_cast<uintptr_t>(pDirectIAT), sizeof(PROC), oldProtect);
        *pDirectIAT = reinterpret_cast<PROC>(Hook_SHGetFolderPathA);
        MemoryTools::RestoreProtect(reinterpret_cast<uintptr_t>(pDirectIAT), sizeof(PROC), oldProtect);
    }

    // ------------------------------------------------------------------------
    // Feature 2: WriteSettingsToFile (Registry Settings Redirection)
    // ------------------------------------------------------------------------
    inline bool s_WriteSettingsToFile = false;
    inline std::filesystem::path s_SettingsIniPath;
    inline std::string s_Filter = "Need for Speed";
    inline thread_local std::string s_CurrentSection = "Need for Speed Most Wanted";
    inline std::map<std::string, std::string> s_DefaultStrings;

    using PFN_RegCreateKeyA = LSTATUS(WINAPI*)(HKEY, LPCSTR, PHKEY);
    using PFN_RegOpenKeyA = LSTATUS(WINAPI*)(HKEY, LPCSTR, PHKEY);
    using PFN_RegOpenKeyExA = LSTATUS(WINAPI*)(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
    using PFN_RegQueryValueExA = LSTATUS(WINAPI*)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
    using PFN_RegSetValueExA = LSTATUS(WINAPI*)(HKEY, LPCSTR, DWORD, DWORD, const BYTE*, DWORD);
    using PFN_RegCloseKey = LSTATUS(WINAPI*)(HKEY);

    inline PFN_RegCreateKeyA s_OrigRegCreateKeyA = nullptr;
    inline PFN_RegOpenKeyA s_OrigRegOpenKeyA = nullptr;
    inline PFN_RegOpenKeyExA s_OrigRegOpenKeyExA = nullptr;
    inline PFN_RegQueryValueExA s_OrigRegQueryValueExA = nullptr;
    inline PFN_RegSetValueExA s_OrigRegSetValueExA = nullptr;
    inline PFN_RegCloseKey s_OrigRegCloseKey = nullptr;

    inline LSTATUS WINAPI Hook_RegCloseKey(HKEY hKey) {
        if (hKey == NULL) return ERROR_SUCCESS;
        if (s_OrigRegCloseKey) return s_OrigRegCloseKey(hKey);
        return RegCloseKey(hKey);
    }

    inline LSTATUS WINAPI Hook_RegCreateKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult) {
        if (hKey == NULL) {
            if (phkResult) *phkResult = NULL;
            return ERROR_SUCCESS;
        }
        if (lpSubKey && (strstr(lpSubKey, s_Filter.c_str()) != NULL || strstr(lpSubKey, "ergc") != NULL)) {
            if (phkResult) *phkResult = NULL;
            std::string sub(lpSubKey);
            auto pos = sub.find_last_of('\\');
            s_CurrentSection = (pos == std::string::npos) ? sub : sub.substr(pos + 1);
            return ERROR_SUCCESS;
        }
        if (s_OrigRegCreateKeyA) return s_OrigRegCreateKeyA(hKey, lpSubKey, phkResult);
        return RegCreateKeyA(hKey, lpSubKey, phkResult);
    }

    inline LSTATUS WINAPI Hook_RegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult) {
        if (hKey == NULL) {
            if (phkResult) *phkResult = NULL;
            return ERROR_SUCCESS;
        }
        if (lpSubKey && (strstr(lpSubKey, s_Filter.c_str()) != NULL || strstr(lpSubKey, "ergc") != NULL)) {
            if (phkResult) *phkResult = NULL;
            std::string sub(lpSubKey);
            auto pos = sub.find_last_of('\\');
            s_CurrentSection = (pos == std::string::npos) ? sub : sub.substr(pos + 1);
            return ERROR_SUCCESS;
        }
        if (s_OrigRegOpenKeyA) return s_OrigRegOpenKeyA(hKey, lpSubKey, phkResult);
        return RegOpenKeyA(hKey, lpSubKey, phkResult);
    }

    inline LSTATUS WINAPI Hook_RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult) {
        if (hKey == NULL) {
            if (phkResult) *phkResult = NULL;
            return ERROR_SUCCESS;
        }
        if (lpSubKey && (strstr(lpSubKey, s_Filter.c_str()) != NULL || strstr(lpSubKey, "ergc") != NULL)) {
            if (phkResult) *phkResult = NULL;
            std::string sub(lpSubKey);
            auto pos = sub.find_last_of('\\');
            s_CurrentSection = (pos == std::string::npos) ? sub : sub.substr(pos + 1);
            return ERROR_SUCCESS;
        }
        if (s_OrigRegOpenKeyExA) return s_OrigRegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
        return RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
    }

    inline LSTATUS WINAPI Hook_RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
        if (hKey != NULL) {
            if (s_OrigRegQueryValueExA) return s_OrigRegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
            return RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
        }

        std::string valName = lpValueName ? lpValueName : "@";

        // Return 0 in memory for legacy installer keys so they never touch Settings.ini
        if (valName == "FirstTime" || valName == "VERSION" || valName == "SIZE") {
            if (lpType) *lpType = REG_DWORD;
            constexpr DWORD required = sizeof(DWORD);
            if (lpData == nullptr) {
                if (lpcbData) *lpcbData = required;
                return ERROR_SUCCESS;
            }
            if (*lpcbData < required) {
                *lpcbData = required;
                return ERROR_MORE_DATA;
            }
            *reinterpret_cast<DWORD*>(lpData) = 0;
            *lpcbData = required;
            return ERROR_SUCCESS;
        }

        // Determine if value is a REG_DWORD or REG_SZ based on key name (lpType is OUT-only from caller)
        bool isDword = (valName.rfind("g_", 0) == 0);

        auto it = s_DefaultStrings.find(valName);
        std::string defaultValue = (it != s_DefaultStrings.end()) ? it->second : std::string{};

        char buf[512] = { 0 };
        GetPrivateProfileStringA(s_CurrentSection.c_str(), valName.c_str(), defaultValue.c_str(), buf, sizeof(buf), s_SettingsIniPath.string().c_str());
        std::string readVal(buf);

        // Special handling for g_RacingResolution: always honor the mod's active resolution index
        // which was determined from NFSMW_WideScreenFix.ini (ResX/ResY) on startup
        if (valName == "g_RacingResolution") {
            int resolvedIndex = Resolution::g_ActiveResIndex;

            if (lpType) *lpType = REG_DWORD;
            if (lpcbData) {
                if (lpData) {
                    if (*lpcbData < sizeof(DWORD)) {
                        *lpcbData = sizeof(DWORD);
                        return ERROR_MORE_DATA;
                    }
                    *reinterpret_cast<DWORD*>(lpData) = static_cast<DWORD>(resolvedIndex);
                }
                *lpcbData = sizeof(DWORD);
                return ERROR_SUCCESS;
            }
            return ERROR_SUCCESS;
        }

        if (isDword) {
            DWORD dwordVal = 0;
            if (!readVal.empty()) {
                dwordVal = static_cast<DWORD>(std::strtoul(readVal.c_str(), nullptr, 10));
            }
            else if (!defaultValue.empty()) {
                dwordVal = static_cast<DWORD>(std::strtoul(defaultValue.c_str(), nullptr, 10));
            }

            if (lpType) *lpType = REG_DWORD;
            constexpr DWORD required = sizeof(DWORD);

            if (lpData == nullptr) {
                if (lpcbData) *lpcbData = required;
                return ERROR_SUCCESS;
            }

            if (*lpcbData < required) {
                *lpcbData = required;
                return ERROR_MORE_DATA;
            }

            *reinterpret_cast<DWORD*>(lpData) = dwordVal;
            *lpcbData = required;
            return ERROR_SUCCESS;
        }
        else {
            // String (REG_SZ)
            if (lpType) *lpType = REG_SZ;
            DWORD required = static_cast<DWORD>(readVal.length() + 1);

            if (lpData == nullptr) {
                if (lpcbData) *lpcbData = required;
                return ERROR_SUCCESS;
            }

            if (*lpcbData < required) {
                *lpcbData = required;
                return ERROR_MORE_DATA;
            }

            if (!readVal.empty()) {
                std::memcpy(lpData, readVal.c_str(), readVal.length());
            }
            lpData[readVal.length()] = '\0';
            *lpcbData = required;
            return ERROR_SUCCESS;
        }
    }

    inline LSTATUS WINAPI Hook_RegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE* lpData, DWORD cbData) {
        if (hKey != NULL) {
            if (s_OrigRegSetValueExA) return s_OrigRegSetValueExA(hKey, lpValueName, Reserved, dwType, lpData, cbData);
            return RegSetValueExA(hKey, lpValueName, Reserved, dwType, lpData, cbData);
        }

        if (!lpValueName || !lpData) return ERROR_INVALID_PARAMETER;

        std::string valName = lpValueName;

        // Only persist actual in-game graphics settings (g_*) to Settings.ini
        if (valName.rfind("g_", 0) != 0) {
            return ERROR_SUCCESS;
        }

        std::string valStr;

        if (dwType == REG_DWORD) {
            DWORD dwVal = *reinterpret_cast<const DWORD*>(lpData);
            if (valName == "g_RacingResolution") {
                int index = static_cast<int>(dwVal);
                if (index >= 0 && index < static_cast<int>(Resolution::g_ResolutionList.size())) {
                    int w = Resolution::g_ResolutionList[index].width;
                    int h = Resolution::g_ResolutionList[index].height;
                    dwVal = (static_cast<DWORD>(w) << 16) | (static_cast<DWORD>(h) & 0xFFFF) | 0x80000000;
                    Resolution::g_ActiveResIndex = index;
                    Resolution::g_CurrentWidth = w;
                    Resolution::g_CurrentHeight = h;
                    Resolution::g_CurrentAspectRatio = static_cast<float>(w) / static_cast<float>(h);
                    if (!g_Config.IniPath.empty()) {
                        WritePrivateProfileStringA("DISPLAY", "ResX", std::to_string(w).c_str(), g_Config.IniPath.string().c_str());
                        WritePrivateProfileStringA("DISPLAY", "ResY", std::to_string(h).c_str(), g_Config.IniPath.string().c_str());
                    }

                    if (Resolution::g_WindowMode > 0) {
                        HWND hWnd = *reinterpret_cast<HWND*>(0x00982BF4);
                        if (!hWnd) hWnd = Resolution::g_GameHWND;
                        if (hWnd) {
                            Resolution::ApplyWindowStyleAndPosition(hWnd, w, h);
                        }
                    }
                }
            }
            valStr = std::to_string(dwVal);
        }
        else {
            valStr = reinterpret_cast<const char*>(lpData);
        }

        WritePrivateProfileStringA(s_CurrentSection.c_str(), valName.c_str(), valStr.c_str(), s_SettingsIniPath.string().c_str());
        return ERROR_SUCCESS;
    }

    inline void InitRegistryRedirect(const std::filesystem::path& gamePath, bool enabled) {
        if (!enabled) return;

        s_WriteSettingsToFile = true;

        if (s_UseCustomUserDir && !s_CustomUserDir.empty()) {
            s_SettingsIniPath = s_CustomUserDir / "NFS Most Wanted" / "Settings.ini";
        }
        else {
            char szDocs[MAX_PATH] = { 0 };
            SHGetFolderPathA(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, szDocs);
            s_SettingsIniPath = std::filesystem::path(szDocs) / "NFS Most Wanted" / "Settings.ini";
        }

        std::error_code ec;
        std::filesystem::create_directories(s_SettingsIniPath.parent_path(), ec);

        // Default graphics settings for Settings.ini
        s_DefaultStrings["g_CarEnvironmentMapEnable"] = "3";
        s_DefaultStrings["g_CarEnvironmentMapUpdateData"] = "1";
        s_DefaultStrings["g_RoadReflectionEnable"] = "3";
        s_DefaultStrings["g_MotionBlurEnable"] = "1";
        s_DefaultStrings["g_ParticleSystemEnable"] = "1";
        s_DefaultStrings["g_WorldLodLevel"] = "3";
        s_DefaultStrings["g_CarLodLevel"] = "1";
        s_DefaultStrings["g_OverBrightEnable"] = "1";
        s_DefaultStrings["g_FSAALevel"] = "3";
        s_DefaultStrings["g_RainEnable"] = "1";
        s_DefaultStrings["g_TextureFiltering"] = "2";
        s_DefaultStrings["g_RacingResolution"] = "1";
        s_DefaultStrings["g_PerformanceLevel"] = "5";
        s_DefaultStrings["g_VSyncOn"] = "0";
        s_DefaultStrings["g_ShadowDetail"] = "2";
        s_DefaultStrings["g_VisualTreatment"] = "1";

        // 3. Resolve original ADVAPI32 functions
        HMODULE hAdvapi32 = GetModuleHandleA("ADVAPI32.dll");
        if (!hAdvapi32) hAdvapi32 = LoadLibraryA("ADVAPI32.dll");
        if (hAdvapi32) {
            s_OrigRegCreateKeyA = reinterpret_cast<PFN_RegCreateKeyA>(GetProcAddress(hAdvapi32, "RegCreateKeyA"));
            s_OrigRegOpenKeyExA = reinterpret_cast<PFN_RegOpenKeyExA>(GetProcAddress(hAdvapi32, "RegOpenKeyExA"));
            s_OrigRegCloseKey = reinterpret_cast<PFN_RegCloseKey>(GetProcAddress(hAdvapi32, "RegCloseKey"));
            s_OrigRegSetValueExA = reinterpret_cast<PFN_RegSetValueExA>(GetProcAddress(hAdvapi32, "RegSetValueExA"));
            s_OrigRegQueryValueExA = reinterpret_cast<PFN_RegQueryValueExA>(GetProcAddress(hAdvapi32, "RegQueryValueExA"));
            s_OrigRegOpenKeyA = reinterpret_cast<PFN_RegOpenKeyA>(GetProcAddress(hAdvapi32, "RegOpenKeyA"));
        }

        // 4. Directly patch speed.exe fixed IAT addresses for ADVAPI32:
        // 0x00890000: RegCreateKeyA
        // 0x00890004: RegOpenKeyExA
        // 0x00890008: RegCloseKey
        // 0x0089000C: RegSetValueExA
        // 0x00890010: RegQueryValueExA
        // 0x00890014: RegOpenKeyA
        struct IATEntry {
            uintptr_t addr;
            PROC hookFunc;
        } entries[] = {
            { 0x00890000, reinterpret_cast<PROC>(Hook_RegCreateKeyA) },
            { 0x00890004, reinterpret_cast<PROC>(Hook_RegOpenKeyExA) },
            { 0x00890008, reinterpret_cast<PROC>(Hook_RegCloseKey) },
            { 0x0089000C, reinterpret_cast<PROC>(Hook_RegSetValueExA) },
            { 0x00890010, reinterpret_cast<PROC>(Hook_RegQueryValueExA) },
            { 0x00890014, reinterpret_cast<PROC>(Hook_RegOpenKeyA) },
        };

        for (const auto& e : entries) {
            DWORD oldProtect;
            PROC* pEntry = reinterpret_cast<PROC*>(e.addr);
            MemoryTools::Unprotect(e.addr, sizeof(PROC), oldProtect);
            *pEntry = e.hookFunc;
            MemoryTools::RestoreProtect(e.addr, sizeof(PROC), oldProtect);
        }
    }
}

#endif // NFSMW_WIDESCREENFIX_USERSETTINGS_H
