#pragma once
#ifndef NFSMW_WIDESCREENFIX_RESOLUTION_H
#define NFSMW_WIDESCREENFIX_RESOLUTION_H

#include "MemoryTools.h"
#include "Config.h"
#include <windows.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>

namespace Resolution {
    // Verified from IDA: Resolution and D3D9 device management addresses

    struct Mode {
        int width;
        int height;
        int refreshRate;
    };

    inline std::vector<Mode> g_ResolutionList;
    inline std::unordered_map<int, std::string> g_ResolutionLabels;
    inline std::vector<int> g_ResolutionHashes;
    inline std::vector<int> g_SupportedModes;
    inline std::vector<int> g_SupportedRefreshRates;

    inline int* g_pResolutionHashes = nullptr;
    inline int g_ResolutionCount = 0;

    inline int g_CurrentWidth = 1024;
    inline int g_CurrentHeight = 768;
    inline float g_CurrentAspectRatio = 4.0f / 3.0f;
    inline int g_ActiveResIndex = 0;

    // Callbacks on resolution change
    using ResChangeCallback = void(*)(int width, int height);
    inline std::vector<ResChangeCallback> g_OnResChangeCallbacks;

    inline void RegisterOnResChange(ResChangeCallback cb) {
        g_OnResChangeCallbacks.push_back(cb);
    }

    // Native EA engine hash algorithm (bStringHash at 0x00460C20)
    inline unsigned int StringHash(const char* str) {
        if (!str) return static_cast<unsigned int>(-1);
        int result = -1;
        unsigned char c = *str;
        while (c != 0) {
            result = c + 33 * result;
            c = *(++str);
        }
        return static_cast<unsigned int>(result);
    }

    inline int g_WindowMode = 0;
    inline int g_WindowModeFallback = 1;
    inline HWND g_GameHWND = NULL;

    using PFN_CreateWindowExA = HWND(WINAPI*)(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
    using PFN_SetWindowLongA = LONG(WINAPI*)(HWND, int, LONG);
    using PFN_AdjustWindowRect = BOOL(WINAPI*)(LPRECT, DWORD, BOOL);
    using PFN_SetWindowPos = BOOL(WINAPI*)(HWND, HWND, int, int, int, int, UINT);

    inline PFN_CreateWindowExA s_OrigCreateWindowExA = nullptr;
    inline PFN_SetWindowLongA s_OrigSetWindowLongA = nullptr;
    inline PFN_AdjustWindowRect s_OrigAdjustWindowRect = nullptr;
    inline PFN_SetWindowPos s_OrigSetWindowPos = nullptr;

    inline void __cdecl ResetDeviceHook();
    inline void __cdecl Hook_sub_6DB0D0() { ResetDeviceHook(); }

    inline void ApplyWindowStyleAndPosition(HWND hWnd, int width, int height) {
        if (!hWnd) return;

        int deskW = GetSystemMetrics(SM_CXSCREEN);
        int deskH = GetSystemMetrics(SM_CYSCREEN);

        int winW = (width > 0) ? width : g_CurrentWidth;
        int winH = (height > 0) ? height : g_CurrentHeight;

        if (g_WindowMode == 4) { // Borderless Fullscreen (maintains aspect ratio)
            float aspect = static_cast<float>(width) / static_cast<float>(height);
            float deskAspect = static_cast<float>(deskW) / static_cast<float>(deskH);
            if (aspect >= deskAspect) {
                winW = deskW;
                winH = static_cast<int>(deskW / aspect);
            } else {
                winH = deskH;
                winW = static_cast<int>(deskH * aspect);
            }
        } else if (g_WindowMode == 5) { // Borderless Fullscreen Stretched (fills screen)
            winW = deskW;
            winH = deskH;
        }

        LONG style = GetWindowLongA(hWnd, GWL_STYLE);
        style &= ~(WS_HSCROLL | WS_VSCROLL);

        if (g_WindowMode == 1 || g_WindowMode == 4 || g_WindowMode == 5) {
            // Borderless / Borderless Fullscreen / Borderless Fullscreen Stretched
            style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
            style |= WS_POPUP;
        } else if (g_WindowMode == 2) {
            // Border
            style |= (WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
            style &= ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
        } else if (g_WindowMode == 3) {
            // Resizable Border
            style |= (WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_MAXIMIZEBOX | WS_THICKFRAME);
        }

        SetWindowLongA(hWnd, GWL_STYLE, style);

        LONG exStyle = GetWindowLongA(hWnd, GWL_EXSTYLE);
        RECT rect = { 0, 0, winW, winH };
        if (style & WS_CAPTION) {
            AdjustWindowRectEx(&rect, style, FALSE, exStyle);
        }

        int totalW = rect.right - rect.left;
        int totalH = rect.bottom - rect.top;
        int left = (deskW - winW) / 2 + rect.left;
        int top = (deskH - winH) / 2 + rect.top;

        RECT* pWindowRect = reinterpret_cast<RECT*>(0x0093DAA4);
        if (pWindowRect) {
            pWindowRect->left = left;
            pWindowRect->top = top;
            pWindowRect->right = left + totalW;
            pWindowRect->bottom = top + totalH;
        }

        SetWindowPos(hWnd, HWND_NOTOPMOST, left, top, totalW, totalH, SWP_SHOWWINDOW | SWP_FRAMECHANGED);
        MoveWindow(hWnd, left, top, totalW, totalH, TRUE);
    }

    inline HWND WINAPI Hook_CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
        if (hWndParent && hWndParent != GetDesktopWindow()) {
            return s_OrigCreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
        }

        SetProcessDPIAware();

        int deskW = GetSystemMetrics(SM_CXSCREEN);
        int deskH = GetSystemMetrics(SM_CYSCREEN);

        int winW = (g_CurrentWidth > 0) ? g_CurrentWidth : ((nWidth > 0) ? nWidth : 1024);
        int winH = (g_CurrentHeight > 0) ? g_CurrentHeight : ((nHeight > 0) ? nHeight : 768);

        if (g_WindowMode == 4) { // Borderless Fullscreen (maintains aspect ratio)
            float aspect = static_cast<float>(winW) / static_cast<float>(winH);
            float deskAspect = static_cast<float>(deskW) / static_cast<float>(deskH);
            if (aspect >= deskAspect) {
                winW = deskW;
                winH = static_cast<int>(deskW / aspect);
            } else {
                winH = deskH;
                winW = static_cast<int>(deskH * aspect);
            }
        } else if (g_WindowMode == 5) { // Borderless Fullscreen Stretched (fills screen)
            winW = deskW;
            winH = deskH;
        }

        int posX = (deskW - winW) / 2;
        int posY = (deskH - winH) / 2;
        if (posX < 0) posX = 0;
        if (posY < 0) posY = 0;

        HWND hWnd = s_OrigCreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, posX, posY, winW, winH, hWndParent, hMenu, hInstance, lpParam);
        g_GameHWND = hWnd;

        if (hWnd) {
            ApplyWindowStyleAndPosition(hWnd, winW, winH);
        }
        return hWnd;
    }

    inline LONG WINAPI Hook_SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong) {
        if (nIndex == GWL_STYLE && g_WindowMode > 0) {
            dwNewLong &= ~(WS_HSCROLL | WS_VSCROLL);
            if (g_WindowMode == 1 || g_WindowMode == 4 || g_WindowMode == 5) {
                dwNewLong &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
                dwNewLong |= WS_POPUP;
            } else if (g_WindowMode == 2) {
                dwNewLong |= (WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
                dwNewLong &= ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
            } else if (g_WindowMode == 3) {
                dwNewLong |= (WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_MAXIMIZEBOX | WS_THICKFRAME);
            }
        }
        return s_OrigSetWindowLongA(hWnd, nIndex, dwNewLong);
    }

    inline BOOL WINAPI Hook_AdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu) {
        if (g_WindowMode == 2) {
            return s_OrigAdjustWindowRect(lpRect, WS_CAPTION, bMenu);
        } else if (g_WindowMode == 3) {
            return s_OrigAdjustWindowRect(lpRect, WS_CAPTION | WS_MAXIMIZEBOX | WS_THICKFRAME, bMenu);
        }
        return s_OrigAdjustWindowRect(lpRect, dwStyle, bMenu);
    }

    inline BOOL WINAPI Hook_SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) {
        return s_OrigSetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
    }




    inline void EnumerateResolutions(int initialResX, int initialResY) {
        std::vector<Mode> modes;
        DEVMODEA dm = { 0 };
        dm.dmSize = sizeof(dm);

        DEVMODEA dmCurrent = { 0 };
        dmCurrent.dmSize = sizeof(dmCurrent);
        int desktopHz = 60;
        if (EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dmCurrent) && dmCurrent.dmDisplayFrequency > 0) {
            desktopHz = static_cast<int>(dmCurrent.dmDisplayFrequency);
        }

        for (DWORD i = 0; EnumDisplaySettingsA(NULL, i, &dm); ++i) {
            if (dm.dmBitsPerPel != 32) continue;
            if (dm.dmPelsWidth < 640 || dm.dmPelsHeight < 480) continue;
            modes.push_back({ static_cast<int>(dm.dmPelsWidth), static_cast<int>(dm.dmPelsHeight), static_cast<int>(dm.dmDisplayFrequency) });
        }

        // Sort: width asc, height asc, refresh rate desc (highest refresh rate first)
        std::sort(modes.begin(), modes.end(), [](const Mode& a, const Mode& b) {
            if (a.width != b.width) return a.width < b.width;
            if (a.height != b.height) return a.height < b.height;
            return a.refreshRate > b.refreshRate;
        });

        // Deduplicate resolutions, preserving highest refresh rate
        auto it = std::unique(modes.begin(), modes.end(), [](const Mode& a, const Mode& b) {
            return a.width == b.width && a.height == b.height;
        });
        modes.erase(it, modes.end());

        // Sort by width asc, height asc
        std::sort(modes.begin(), modes.end(), [](const Mode& a, const Mode& b) {
            if (a.width != b.width) return a.width < b.width;
            return a.height < b.height;
        });

        if (modes.empty()) {
            modes.push_back({ 1920, 1080, desktopHz });
        }

        g_ResolutionList.clear();
        g_ResolutionLabels.clear();
        g_ResolutionHashes.clear();
        g_SupportedModes.clear();
        g_SupportedRefreshRates.clear();


        for (const auto& m : modes) {
            g_ResolutionList.push_back(m);
            std::string label = std::to_string(m.width) + "x" + std::to_string(m.height);
            std::string hashStr = "OPT_VO_PC_RES_" + std::to_string(m.width) + "X" + std::to_string(m.height);
            int hash = static_cast<int>(StringHash(hashStr.c_str()));

            g_ResolutionLabels[hash] = label;
            g_ResolutionHashes.push_back(hash);
            g_SupportedModes.push_back(1);
            g_SupportedRefreshRates.push_back(m.refreshRate);
        }

        // Determine target starting resolution
        int targetW = initialResX;
        int targetH = initialResY;
        if (targetW <= 0 || targetH <= 0) {
            targetW = GetSystemMetrics(SM_CXSCREEN);
            targetH = GetSystemMetrics(SM_CYSCREEN);
        }

        g_ActiveResIndex = -1;
        for (size_t i = 0; i < g_ResolutionList.size(); ++i) {
            if (g_ResolutionList[i].width == targetW && g_ResolutionList[i].height == targetH) {
                g_ActiveResIndex = static_cast<int>(i);
                break;
            }
        }

        if (g_ActiveResIndex == -1) {
            Mode m{ targetW, targetH, desktopHz };
            g_ResolutionList.push_back(m);
            std::string label = std::to_string(m.width) + "x" + std::to_string(m.height);
            std::string hashStr = "OPT_VO_PC_RES_" + std::to_string(m.width) + "X" + std::to_string(m.height);
            int hash = static_cast<int>(StringHash(hashStr.c_str()));

            g_ResolutionLabels[hash] = label;
            g_ResolutionHashes.push_back(hash);
            g_SupportedModes.push_back(1);
            g_SupportedRefreshRates.push_back(desktopHz);
            g_ActiveResIndex = static_cast<int>(g_ResolutionList.size() - 1);
        }

        g_CurrentWidth = g_ResolutionList[g_ActiveResIndex].width;
        g_CurrentHeight = g_ResolutionList[g_ActiveResIndex].height;
        g_CurrentAspectRatio = static_cast<float>(g_CurrentWidth) / static_cast<float>(g_CurrentHeight);
    }

    inline DWORD GetPackedActiveRes() {
        if (g_ActiveResIndex >= 0 && g_ActiveResIndex < static_cast<int>(g_ResolutionList.size())) {
            int w = g_ResolutionList[g_ActiveResIndex].width;
            int h = g_ResolutionList[g_ActiveResIndex].height;
            return (static_cast<DWORD>(w) << 16) | (static_cast<DWORD>(h) & 0xFFFF) | 0x80000000;
        }
        return 0;
    }

    // Native GetResolution hooks (0x006C27D0 and 0x006C28B0)
    // Converts g_RacingResolution index or packed resolution format into width & height
    inline void __stdcall GetResolutionHook(int* outWidth, int* outHeight) {
        if (!outWidth || !outHeight) return;

        int idx = *reinterpret_cast<int*>(0x0090181C); // g_RacingResolution

        if (idx & 0x80000000) {
            int w = (idx & 0x7FFF0000) >> 16;
            int h = idx & 0xFFFF;
            int found = -1;
            for (size_t i = 0; i < g_ResolutionList.size(); ++i) {
                if (g_ResolutionList[i].width == w && g_ResolutionList[i].height == h) {
                    found = static_cast<int>(i);
                    break;
                }
            }
            idx = (found != -1) ? found : g_ActiveResIndex;
            *reinterpret_cast<int*>(0x0090181C) = idx;
        }

        if (idx < 0 || idx >= static_cast<int>(g_ResolutionList.size())) {
            idx = g_ActiveResIndex;
            *reinterpret_cast<int*>(0x0090181C) = idx;
        }

        g_ActiveResIndex = idx;
        *outWidth = g_ResolutionList[idx].width;
        *outHeight = g_ResolutionList[idx].height;
        g_CurrentWidth = *outWidth;
        g_CurrentHeight = *outHeight;
        g_CurrentAspectRatio = static_cast<float>(*outWidth) / static_cast<float>(*outHeight);

        if (g_WindowMode > 0) {
            HWND hWnd = *reinterpret_cast<HWND*>(0x00982BF4);
            if (!hWnd) hWnd = g_GameHWND;
            if (hWnd) {
                RECT cr = { 0 };
                if (GetClientRect(hWnd, &cr)) {
                    int curW = cr.right - cr.left;
                    int curH = cr.bottom - cr.top;
                    if (curW > 0 && curH > 0 && (curW != *outWidth || curH != *outHeight)) {
                        ApplyWindowStyleAndPosition(hWnd, *outWidth, *outHeight);
                    }
                }
            }
        }
    }

    inline void __stdcall Hook_GetRacingResolution(int* outWidth, int* outHeight) {
        GetResolutionHook(outWidth, outHeight);
    }

    // Native eViewPlatInterface::ResetDevice hook (0x006DB0D0)
    // Called after D3D9 device reset to re-layout HUD and notify components
    inline void __cdecl ResetDeviceHook() {
        reinterpret_cast<void(__cdecl*)()>(0x006DB0D0)();

        int width = 0, height = 0;
        GetResolutionHook(&width, &height);

        if (g_WindowMode > 0) {
            HWND hWnd = *reinterpret_cast<HWND*>(0x00982BF4);
            if (!hWnd) hWnd = g_GameHWND;
            ApplyWindowStyleAndPosition(hWnd, width, height);
        }

        for (auto& cb : g_OnResChangeCallbacks) {
            cb(width, height);
        }

        // Notify cFEng UI packages that layout needs recalculation:
        // cFEng::MakeLoadedPackagesDirty (0x00525A00)
        void** pFEng = reinterpret_cast<void**>(0x0091CADC);
        if (pFEng && *pFEng) {
            using MakeLoadedPackagesDirtyFn = void(__thiscall*)(void*, int);
            reinterpret_cast<MakeLoadedPackagesDirtyFn>(0x00525A00)(*pFEng, 0);
        }
    }

    // Native VOScreenResolution::Act hook (0x0050F970)
    // Handles resolution cycling in the in-game Video Options menu
    inline void __fastcall VOScreenResolution_Act_Hook(void* pThis, void* edx, int a2, int actionId) {
        uintptr_t* pDALManager = *reinterpret_cast<uintptr_t**>(0x0091CA20);
        if (!pDALManager) return;

        int* pCurrentIndex = reinterpret_cast<int*>(*reinterpret_cast<uintptr_t*>(pDALManager) + 108);
        int currentIndex = *pCurrentIndex;

        constexpr int ACTION_DECREMENT = static_cast<int>(0x9120409E);
        constexpr int ACTION_INCREMENT = static_cast<int>(0xB5971BF1);

        int delta = 0;
        bool changed = false;

        if (actionId == ACTION_DECREMENT) {
            delta = -1;
            changed = true;
        } else if (actionId == ACTION_INCREMENT) {
            delta = 1;
            changed = true;
        }

        if (changed) {
            int count = static_cast<int>(g_ResolutionList.size());
            do {
                currentIndex += delta;
                if (currentIndex < 0) currentIndex = count - 1;
                else if (currentIndex >= count) currentIndex = 0;
            } while (!g_SupportedModes[currentIndex]);

            *pCurrentIndex = currentIndex;
        }

        // Mark widget modified: m_bModified at offset 42 (0x2A)
        reinterpret_cast<uint8_t*>(pThis)[42] = 1;
        uintptr_t* vtable = *reinterpret_cast<uintptr_t**>(pThis);

        // Virtual method 14 (offset 0x38): NotifyAction(actionId)
        using NotifyFn = void(__thiscall*)(void*, int);
        reinterpret_cast<NotifyFn>(vtable[14])(pThis, actionId);

        // Virtual method 3 (offset 0x0C): Draw()
        using UpdateFn = void(__thiscall*)(void*);
        reinterpret_cast<UpdateFn>(vtable[3])(pThis);
    }

    inline void __fastcall Hook_IncScreenRes(void* pThis, void* edx, int a2, int actionId) {
        VOScreenResolution_Act_Hook(pThis, edx, a2, actionId);
    }

    inline int GetMenuResolutionHash(int ecxIndex) {
        int index = ecxIndex;
        uintptr_t* pDALManager = *reinterpret_cast<uintptr_t**>(0x0091CA20);
        if (pDALManager) {
            uintptr_t p1 = *pDALManager;
            if (p1) {
                int idx = *reinterpret_cast<int*>(p1 + 108);
                if (idx >= 0 && idx < g_ResolutionCount) {
                    index = idx;
                }
            }
        }
        if (index >= 0 && index < g_ResolutionCount && g_pResolutionHashes) {
            return g_pResolutionHashes[index];
        }
        return 0;
    }

    // Mid-hook inside VOScreenResolution::Draw (0x0051B7A0 at 0x0051B818)
    // Replaces the vanilla 640..1600 resolution string hash with custom hash in EAX
    inline void __declspec(naked) VOScreenResolution_Draw_HashHook() {
        __asm {
            push ecx
            call GetMenuResolutionHash
            add esp, 4
            // Re-execute replaced instructions at 0x0051B818:
            // 8B 77 30: mov esi, [edi + 0x30]  (ecx0[12]: value FEString)
            // 85 F6:    test esi, esi
            mov esi, dword ptr [edi + 0x30]
            test esi, esi
            ret
        }
    }

    inline void __declspec(naked) Hook_GetMenuResText() {
        VOScreenResolution_Draw_HashHook();
    }

    // Hook GetLocalizedWideString (0x0057E920) to translate custom resolution hashes
    using PFN_GetLocalizedWideString = char(__cdecl*)(wchar_t*, int, unsigned int);

    inline char __cdecl Hook_GetLocalizedWideString(wchar_t* out, int size, unsigned int hash) {
        auto it = g_ResolutionLabels.find(static_cast<int>(hash));
        if (it != g_ResolutionLabels.end()) {
            std::wstring ws(it->second.begin(), it->second.end());
            wcsncpy_s(out, size, ws.c_str(), _TRUNCATE);
            return 1;
        }
        return reinterpret_cast<PFN_GetLocalizedWideString>(0x0057E920)(out, size, hash);
    }

    inline void Init(int initialResX, int initialResY, int windowMode) {
        EnumerateResolutions(initialResX, initialResY);

        g_pResolutionHashes = g_ResolutionHashes.data();
        g_ResolutionCount = static_cast<int>(g_ResolutionList.size());

        // 1. Set initial resolution index in game engine (g_RacingResolution at 0x0090181C)
        *reinterpret_cast<int*>(0x0090181C) = g_ActiveResIndex;

        // 2. Redirect supported resolutions array pointer (originally 0x0093DBEC)
        uintptr_t pResArray = reinterpret_cast<uintptr_t>(g_SupportedModes.data());
        MemoryTools::Write<uintptr_t>(0x0050F9B2, pResArray); // VOScreenResolution::Act
        MemoryTools::Write<uintptr_t>(0x006C195C, pResArray); // sub_6C1780
        MemoryTools::Write<uintptr_t>(0x006C1973, pResArray); // sub_6C1780

        // 3. Redirect supported refresh rates array pointer (originally 0x009829FC)
        uintptr_t pRefArray = reinterpret_cast<uintptr_t>(g_SupportedRefreshRates.data());
        MemoryTools::Write<uintptr_t>(0x006C2898, pRefArray);
        MemoryTools::Write<uintptr_t>(0x006C2978, pRefArray);
        MemoryTools::Write<uintptr_t>(0x006DB1B7, pRefArray);
        MemoryTools::Write<uintptr_t>(0x006DB21E, pRefArray);
        MemoryTools::Write<uintptr_t>(0x006DB2EF, pRefArray);
        MemoryTools::Write<uintptr_t>(0x006DE163, pRefArray);
        MemoryTools::Write<uintptr_t>(0x006E6BC7, pRefArray);
        MemoryTools::Write<uintptr_t>(0x006E72AA, pRefArray);

        // 4. Bypass video autodetection overwrite of g_RacingResolution in AutoDetectVideoSettings (0x006C1990)
        MemoryTools::MakeNOP(0x006C19A4, 6); // mov g_RacingResolution, 0
        MemoryTools::MakeNOP(0x006C1ABA, 6); // mov g_RacingResolution, 1

        // 5. Hook native GetResolution functions: 0x006C27D0 & 0x006C28B0
        MemoryTools::MakeJMP(0x006C27D0, reinterpret_cast<uintptr_t>(GetResolutionHook));
        MemoryTools::MakeJMP(0x006C28B0, reinterpret_cast<uintptr_t>(GetResolutionHook));

        // 6. Hook VOScreenResolution::Act: 0x0050F970
        MemoryTools::MakeJMP(0x0050F970, reinterpret_cast<uintptr_t>(VOScreenResolution_Act_Hook));

        // 7. Hook VOScreenResolution::Draw resolution string hash loader: 0x0051B818
        MemoryTools::MakeCALL(0x0051B818, reinterpret_cast<uintptr_t>(VOScreenResolution_Draw_HashHook));

        // 8. Hook GetLocalizedWideString at all call sites to format custom resolution labels:
        // 0x00515064: Options validation
        // 0x005151D4: Options update
        // 0x00524F70: FEng text drawing
        // 0x005A442D: Menu option text renderer
        MemoryTools::MakeCALL(0x00515064, reinterpret_cast<uintptr_t>(Hook_GetLocalizedWideString));
        MemoryTools::MakeCALL(0x005151D4, reinterpret_cast<uintptr_t>(Hook_GetLocalizedWideString));
        MemoryTools::MakeCALL(0x00524F70, reinterpret_cast<uintptr_t>(Hook_GetLocalizedWideString));
        MemoryTools::MakeCALL(0x005A442D, reinterpret_cast<uintptr_t>(Hook_GetLocalizedWideString));

        // 9. Hook eViewPlatInterface::ResetDevice (0x006DB0D0) at all 5 call sites
        MemoryTools::MakeCALL(0x006DB302, reinterpret_cast<uintptr_t>(ResetDeviceHook));
        MemoryTools::MakeCALL(0x006DE185, reinterpret_cast<uintptr_t>(ResetDeviceHook));
        MemoryTools::MakeCALL(0x006E72C1, reinterpret_cast<uintptr_t>(ResetDeviceHook));
        MemoryTools::MakeCALL(0x006E753E, reinterpret_cast<uintptr_t>(ResetDeviceHook));
        MemoryTools::MakeCALL(0x006E7606, reinterpret_cast<uintptr_t>(ResetDeviceHook));

        // 10. Window Mode support: 0 = Fullscreen | 1 = Borderless | 2 = Border | 3 = Resizable Border | 4 = Borderless Fullscreen | 5 = Borderless Fullscreen Stretched
        g_WindowMode = windowMode;
        g_WindowModeFallback = g_WindowMode > 0 ? g_WindowMode : 1;


        if (g_WindowMode > 0) {
            *reinterpret_cast<uint32_t*>(0x00982BF0) = 1; // WindowedMode flag (0x00982BF0)

            // Hook USER32 IAT entries in speed.exe
            s_OrigCreateWindowExA = *reinterpret_cast<PFN_CreateWindowExA*>(0x008903A4);
            s_OrigSetWindowLongA = *reinterpret_cast<PFN_SetWindowLongA*>(0x00890320);
            s_OrigAdjustWindowRect = *reinterpret_cast<PFN_AdjustWindowRect*>(0x008903A0);
            s_OrigSetWindowPos = *reinterpret_cast<PFN_SetWindowPos*>(0x0089031C);

            MemoryTools::Write<void*>(0x008903A4, reinterpret_cast<void*>(Hook_CreateWindowExA));
            MemoryTools::Write<void*>(0x00890320, reinterpret_cast<void*>(Hook_SetWindowLongA));
            MemoryTools::Write<void*>(0x008903A0, reinterpret_cast<void*>(Hook_AdjustWindowRect));
            MemoryTools::Write<void*>(0x0089031C, reinterpret_cast<void*>(Hook_SetWindowPos));
        }
    }
}

#endif // NFSMW_WIDESCREENFIX_RESOLUTION_H
