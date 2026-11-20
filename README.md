# NFSMW_WideScreenMode

A lightweight, modern widescreen plugin for **Need for Speed: Most Wanted (2005) v1.3**, built from the ground up with clean C++ and reverse engineering in IDA Pro.

---

## Features

### 🖥️ Display & Resolution

- **In-Game Resolution Picker:** Dynamically enumerates all monitor display modes directly into the in-game Video Options menu with seamless runtime switching.
- **Windowed & Borderless Modes:** Supports Fullscreen, Borderless, Bordered, Resizable, and Borderless Fullscreen Stretched modes.

### 🎯 FOV & HUD

- **Hor+ Camera FOV:** Automatically calculates and applies correct field of view for any aspect ratio.
- **Aspect-Correct HUD:** Fixes HUD stretching and moves HUD elements to widescreen screen edges.

### 🎬 Movies & FMVs

- **Widescreen FMV Scaling:** Options to zoom/fill 16:9 or maintain aspect ratio with clean pillarboxing (`WideScreenMovie`).
- **Movie Region Auto-Detection:** Automatically detects and loads NTSC (`_ntsc`) or PAL (`_pal`) movie formats, with optional manual override (`MovieType`).
- **Skip Intro:** Bypasses startup logos and intro movies (`SkipIntro`).
- **Full-Color Range Videos:** Custom YUV lookup table expansion restores deep blacks and full color vibrancy (0–255) instead of washed-out limited range (16–235).

### 🔊 Audio

- **High-Spec Audio:** Forces the audio mixer to run at full CD quality (44,100 Hz) by default, bypassing 22,050 Hz low-spec fallbacks.

### 🏎️ Graphics & Visuals

- **High-Precision SimRate:** Decoupled physics simulation ticker supporting Dynamic mode (matches monitor refresh rate — 144Hz, 165Hz, 240Hz, etc.), Fixed 200 Hz mode, or vanilla 60 Hz.
- **FE Garage 60 FPS Cap (`LockGarageFPS`):** Dedicated frame limiter keeping the safehouse garage, car lot, and customization shop locked at native 60 FPS for authentic turntable rotation and UI animations.
- **GPU Vendor Emulation:** Override PCI Vendor ID (Nvidia, AMD, Intel) to enable vendor-specific features and fixes.
- **Dynamic Shadows:** Customizable shadow map resolution (up to 16384), auto-scaling based on aspect ratio, increased shadow LOD detail, and tunnel/bridge disappearance fix.
- **Console Gamma & Motion Blur:** Console / MW gamma modes and in-game motion blur toggle (automatically disabled and greyed out when X360Stuff is installed).

### ⚙️ Miscellaneous

- **Windows Cursor Toggle:** Optional hardware Windows OS cursor in menus without software cursor latency or stretching (`Win Cursor`).
- **Portable Saves & Registry:** Redirects game settings from Windows Registry into a local file and supports custom save game folders (`WriteSettingsToFile`, `CustomUserFilesDirectoryInGameDir`).
- **Standalone Companion Mods:** Controller support and nitrous trail enhancements have been decoupled into their own dedicated companion plugins:
  - [**NFSMW_ControllerSupport**](https://github.com/CiPH3R-88/NFSMW_ControllerSupport) — Full XInput, DualShock 4 / DualSense button icons, deadzone tuning, and analog stick fixes.
  - [**NFSMW_NOSTrailFix**](https://github.com/CiPH3R-88/NFSMW_NOSTrailFix) — High-FPS nitrous trail fixes, custom length scaling, tip fade-out, and 360° pause camera visibility.
- **Mod Compatibility:** Works out of the box with [**NFSMW_ControllerSupport**](https://github.com/CiPH3R-88/NFSMW_ControllerSupport), [**NFSMW_NOSTrailFix**](https://github.com/CiPH3R-88/NFSMW_NOSTrailFix), **X360Stuff**, and **Sunset Mod** with automatic conflict resolution.

---

## Installation

1. Copy `NFSMW_WideScreenMode.asi` and `NFSMW_WideScreenMode.ini` into your game's `scripts/` folder (requires an ASI Loader - [`dinput8.dll`](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases)).
2. Configure settings inside `NFSMW_WideScreenMode.ini` to your preference.

---

## Configuration Overview

| Section      | Key Settings                                                                                                         |
| :----------- | :------------------------------------------------------------------------------------------------------------------- |
| `[DISPLAY]`  | `ResX`, `ResY`, `WindowMode` (0 = Fullscreen, 1 = Borderless, 2 = Border, 3 = Resizable, etc.)                       |
| `[HUD]`      | `FixHUD`, `WideScreenHUD`, `SplashScreen` (0 = Auto, 1 = 4:3, 2 = Widescreen)                                        |
| `[MOVIES]`   | `WideScreenMovie` (1 = Fill 16:9, 2 = Aspect Correct), `MovieType` (0 = Auto, 1 = NTSC, 2 = PAL), `SkipIntro`        |
| `[GRAPHICS]` | `MotionBlur`, `ConsoleGamma`, `GPUVendor`, `SimRate` (1 = Dynamic, 2 = Fixed), `LockGarageFPS`                       |
| `[SHADOW]`   | `ShadowsRes`, `ShadowsFix`, `ImproveShadowLOD`, `AutoScaleShadowsRes`, `ShadowMapTextureFormat`, `ForceSharpShadows` |
| `[MISC]`     | `WriteSettingsToFile`, `CustomUserFilesDirectoryInGameDir`, `Win Cursor`                                             |

---

## Credits

- **nlg** — Providing the IDA Pro database for NFS Most Wanted.
- **Toru** — Full-color YUV LUT calculation logic and X360Stuff reference.
- **Archie** — Sunset Mod codebase, which helped implement and add the Motion Blur option in the in-game Video Settings.
- **ThirteenAG** — Referred to for initial research.
