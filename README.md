# NFSMW WideScreen Fix

A lightweight, modern widescreen plugin for **Need for Speed: Most Wanted (2005) v1.3**, built from the ground up with clean C++ and reverse engineering in IDA Pro.

---

## Features

### 🖥️ Display & Resolution
* **In-Game Resolution Picker:** Dynamically enumerates all monitor display modes directly into the in-game Video Options menu with seamless runtime switching.
* **Windowed & Borderless Modes:** Supports Fullscreen, Borderless, Bordered, Resizable, and Borderless Fullscreen Stretched modes.

### 🎯 FOV & HUD
* **Hor+ Camera FOV:** Automatically calculates and applies correct field of view for any aspect ratio.
* **Aspect-Correct HUD:** Fixes HUD stretching and moves HUD elements to widescreen screen edges.

### 🎬 Movies & FMVs
* **Widescreen FMV Scaling:** Options to zoom/fill 16:9 or maintain aspect ratio with clean pillarboxing.
* **Full-Color Range Videos:** Custom YUV lookup table expansion restores deep blacks and full color vibrancy (0–255) instead of washed-out limited range (16–235).

### 🔊 Audio
* **High-Spec Audio:** Forces the audio mixer to run at full CD quality (44,100 Hz) by default, bypassing 22,050 Hz low-spec fallbacks.

### 🏎️ Graphics & Visuals
* **Dynamic Shadows:** Customizable shadow map resolution (up to 16384), auto-scaling based on aspect ratio, increased shadow LOD detail, and tunnel/bridge disappearance fix.
* **NOS Trail:** Option to toggle NOS trails on/off or fine-tune trail length and origin offsets.
* **Motion Blur:** In-game motion blur toggle.

### ⚙️ Physics & Miscellaneous
* **High-Precision SimRate:** Decoupled 200 Hz physics simulation ticker for high refresh rate monitors.
* **Skip Intro:** Bypasses startup logos and intro movies.
* **Windows Cursor Toggle:** Optional hardware Windows OS cursor in menus without software cursor latency or stretching.
* **Portable Saves & Registry:** Redirects game settings from Windows Registry into a local file and supports custom save game folders.
* **Mod Compatibility:** Works out of the box with **X360Stuff** and **Sunset Mod** with automatic conflict resolution.

---

## Installation

1. Copy `NFSMW_WideScreenFix.asi` and `NFSMW_WideScreenFix.ini` into your game's `scripts/` folder (requires an ASI Loader like `dinput8.dll`).
2. Configure settings inside `NFSMW_WideScreenFix.ini` to your preference.

---

## Configuration Overview

| Section | Key Settings |
| :--- | :--- |
| `[DISPLAY]` | `ResX`, `ResY`, `WindowMode` (0 = Fullscreen, 1 = Borderless, etc.) |
| `[HUD]` | `FixHUD`, `WideScreenHUD`, `WS_MWSPLASHBACK` |
| `[MOVIES]` | `WideScreenMovie` (1 = Fill 16:9, 2 = Aspect Correct) |
| `[GRAPHICS]`| `MotionBlur`, `ConsoleGamma` |
| `[SHADOW]` | `ShadowsRes`, `ShadowsFix`, `ImproveShadowLOD`, `AutoScaleShadowsRes`, `ShadowMapTextureFormat` |
| `[MISC]` | `SkipIntro`, `SimRate`, `WriteSettingsToFile`, `CustomUserFilesDirectoryInGameDir`, `Win Cursor` |
| `[NOS TRAIL]`| `NosTrail`, `NOSTrailLength`, `NOSTrailPositionScalar` |

---

## Credits

* **Nlg** — Providing the IDA Pro database for NFS Most Wanted.
* **Toru** — Full-color YUV LUT calculation logic and X360Stuff reference.
* **Archie** — Sunset Mod codebase, which helped implement and add the Motion Blur option in the in-game Video Settings.
* **ThirteenAG** — Referred to for initial research.

