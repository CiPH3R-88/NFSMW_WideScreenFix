# NFSMW WideScreen Fix

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

### 🎮 Gamepad & Controller Support

- **Native Xbox & PlayStation Controller Support:** Full support for modern controllers with auto-detection of Xbox (XInput / Microsoft VID) and PlayStation (DualShock 3/4, DualSense 5, Sony VID) layouts.
- **Dynamic In-Game Button Icons:** Injects high-resolution controller button icons (A/B/X/Y, Cross/Circle/Square/Triangle, LB/RB/L1/R1, LT/RT/L2/R2, D-pad, Start, Back/Select) into all UI menus, HUD prompts, and Controls options.
- **Seamless Input Switching:** Automatically switches between Gamepad and Keyboard icons on the fly based on your latest input activity.
- **Full Hotplugging & Disconnect Handling:** Connect or disconnect your controller at any time before or during gameplay. Menus and the in-game Controls tab adapt dynamically without freezing or requiring a restart.
- **Persistent Custom Keybindings:** Your custom controller bindings persist across game sessions and hotplugs (`NFSMW_ControllerBinds.dat` stored beside `Settings.ini`), eliminating the vanilla bug where bindings revert to factory defaults.
- **Simultaneous Keyboard + Controller:** Blends keyboard and gamepad inputs simultaneously — control driving or pause with keyboard keys without locking out the gamepad.
- **In-Game Controller Option:** Adds a Controller toggle directly in the in-game **Gameplay Options** menu (`Off`, `Xbox`, `PlayStation`) when `GamepadSupport = 3` (Auto).
- **Deadzone & Trigger Calibration:** Proportional stick deadzones (`LeftStickDeadzone`, `RightStickDeadzone`) and analog trigger resting filter to eliminate stick drift and unwanted trigger inputs.
- **Quality-of-Life Shortcuts:** Start button pauses the game; pressing Left Stick + Right Stick simultaneously resets your car.

### 🔊 Audio

- **High-Spec Audio:** Forces the audio mixer to run at full CD quality (44,100 Hz) by default, bypassing 22,050 Hz low-spec fallbacks.

### 🏎️ Graphics & Visuals

- **High-Precision SimRate:** Decoupled physics simulation ticker supporting Dynamic mode (matches monitor refresh rate — 144Hz, 165Hz, 240Hz, etc.), Fixed 200 Hz mode, or vanilla 60 Hz.
- **GPU Vendor Emulation:** Override PCI Vendor ID (Nvidia, AMD, Intel) to enable vendor-specific features and fixes.
- **Dynamic Shadows:** Customizable shadow map resolution (up to 16384), auto-scaling based on aspect ratio, increased shadow LOD detail, and tunnel/bridge disappearance fix.
- **Console Gamma & Motion Blur:** Console / MW gamma modes and in-game motion blur toggle.
- **NOS Trail:** Option to toggle NOS trails on/off or fine-tune trail length and origin offsets, with an optional fix to keep trails visible/frozen while paused (`NOSFlareTrailPauseFix`).

### ⚙️ Miscellaneous

- **Windows Cursor Toggle:** Optional hardware Windows OS cursor in menus without software cursor latency or stretching.
- **Portable Saves & Registry:** Redirects game settings from Windows Registry into a local file and supports custom save game folders.
- **Mod Compatibility:** Works out of the box with **X360Stuff** and **Sunset Mod** with automatic conflict resolution.

---

## Installation

1. Copy `NFSMW_WideScreenFix.asi` and `NFSMW_WideScreenFix.ini` into your game's `scripts/` folder (requires an ASI Loader - [`dinput8.dll`](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases)).
2. Configure settings inside `NFSMW_WideScreenFix.ini` to your preference.

---

## Configuration Overview

| Section       | Key Settings                                                                                                          |
| :------------ | :-------------------------------------------------------------------------------------------------------------------- |
| `[DISPLAY]`   | `ResX`, `ResY`, `WindowMode` (0 = Fullscreen, 1 = Borderless, 2 = Border, 3 = Resizable, etc.)                        |
| `[HUD]`       | `FixHUD`, `WideScreenHUD`, `SplashScreen` (0 = Auto, 1 = 4:3, 2 = Widescreen)                                         |
| `[MOVIES]`    | `WideScreenMovie` (1 = Fill 16:9, 2 = Aspect Correct), `MovieType` (0 = Auto, 1 = NTSC, 2 = PAL), `SkipIntro`        |
| `[GRAPHICS]`  | `MotionBlur`, `ConsoleGamma`, `GPUVendor`, `SimRate` (1 = Dynamic, 2 = Fixed 200Hz)                                   |
| `[SHADOW]`    | `ShadowsRes`, `ShadowsFix`, `ImproveShadowLOD`, `AutoScaleShadowsRes`, `ShadowMapTextureFormat`, `ForceSharpShadows` |
| `[MISC]`      | `WriteSettingsToFile`, `CustomUserFilesDirectoryInGameDir`, `Win Cursor`, `GamepadSupport`, `LeftStickDeadzone`, `RightStickDeadzone` |
| `[NOS TRAIL]` | `NosTrail`, `NOSTrailLength`, `NOSTrailPositionScalar`, `NOSFlareTrailPauseFix`                                      |

---

## Credits

- **nlg** — Providing the IDA Pro database for NFS Most Wanted.
- **Toru** — Full-color YUV LUT calculation logic and X360Stuff reference.
- **Archie** — Sunset Mod codebase, which helped implement and add the Motion Blur option in the in-game Video Settings.
- **ThirteenAG** — Referred to for initial research.
