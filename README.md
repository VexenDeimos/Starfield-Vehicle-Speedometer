# Starfield Vehicle Speedometer

**A customizable land-vehicle speedometer for Starfield**  
Created by **VexenDeimos** · **v0.2.2** · Licensed under **GPL-3.0-or-later**

A compact circular speedometer that works as its own SFSE plugin. It does **not** require Starfield Advanced DualSense (SAD), the Map Cursor Meter, DSX, or a controller wrapper.

## Features

- Circular driving HUD with a responsive needle, stable whole-number speed readout, and optional peak-speed indicator.
- **KM/H**, **MPH**, **M/S**, and **U/S** (raw game-world units per second).
- Automatic gauge scaling when changing units, plus adjustable full-scale range.
- Independent customization of needle, numerals, units, rim, tick marks, progress arc, and background.
- Starfield Blue, Starfield White, Amber, and Custom color presets; background transparency from 0% to 100%.
- Preset HUD positions, fine X/Y offsets, and a size slider.
- Automatic hiding in game menus and maps; optional F1-menu preview and outside-vehicle idle gauge.
- Saved preferences, adjustable needle smoothing, optional CSV capture and advanced telemetry/calibration controls.
- Normal telemetry diagnostics **off by default**.

## Requirements

**For playing:**

- A supported Windows PC version of **Starfield**.
- **Starfield Script Extender (SFSE)** compatible with that game version.
- **SFSE Menu Framework**, which supplies the settings menu and HUD overlay.

**For building:** Windows C++23 compiler (MSVC), [xmake](https://xmake.io/), and a separate compatible **CommonLibSF** checkout. These build dependencies are not required by players. SFSE Menu Framework is still required by players; the included SDK header is only used at build time.

> **Compatibility note:** Tested in-game with the author's installed Starfield/SFSE configuration. Specific game-build compatibility has not been exhaustively tested. Install matching versions of Starfield and SFSE.

## Installation

**Vortex or Mod Organizer 2:** Install the **Nexus release ZIP**, which uses the mod-manager-ready path `SFSE/Plugins/StarfieldVehicleSpeedometer.dll` (relative to the game's `Data` folder).

**Manual installation:** Extract `StarfieldVehicleSpeedometer.dll` from that ZIP to:

```text
<Starfield installation>/Data/SFSE/Plugins/StarfieldVehicleSpeedometer.dll
```

Launch the game through SFSE. Open **F1 → Vehicle Speedometer → Options** or **Appearance** to customize the gauge.

**Updating:** Replace only the speedometer DLL. Existing `Data/SFSE/Plugins/VehicleSpeedometer/Settings.ini` settings are preserved; the release ZIP intentionally contains no settings file.

**Uninstalling:** Remove `Data/SFSE/Plugins/StarfieldVehicleSpeedometer.dll`. You may optionally remove `Data/SFSE/Plugins/VehicleSpeedometer/` to delete this plugin's saved settings and CSV files. Never delete unrelated SFSE plugin folders.

## Speed-unit calibration

The default is **1 game world unit = 1 meter**, based on two straight-line REV-8 comparisons against the game's scanner-marker distance:

| Scanner distance change | Recorded raw trip | Absolute difference |
| ---: | ---: | ---: |
| 329.0 m | 329.3 U | 0.3 m (0.09%) |
| 545.0 m | 547.0 U | 2.0 m (0.37%) |

This is **in-game empirical validation**, not an official engine specification. The default is suitable for normal KM/H, MPH, and M/S display; an advanced scale override remains available for experimental setups. The raw U/S mode is unaffected by the metric scale.

## Source build

This source repository is independent of SAD and Map Cursor Meter. CommonLibSF must be obtained separately. For example, in **Windows PowerShell**:

```powershell
$env:COMMONLIBSF_DIR = 'C:\Development\CommonLibSF'
xmake f -c -m release -y
if ($LASTEXITCODE -ne 0) { throw 'xmake configuration failed' }
xmake build StarfieldVehicleSpeedometer
if ($LASTEXITCODE -ne 0) { throw 'Vehicle speedometer build failed' }
```

Use a compatible version of CommonLibSF. The build creates `build/windows/x64/release/StarfieldVehicleSpeedometer.dll` with Windows `FileVersion` **0.2.2.0**.

The standalone source was compiled successfully on Windows (MSVC / Visual Studio 2026). Gameplay testing was performed using the v0.2.2 DLL compiled from the development workspace. Independently compiled DLL hashes may differ because of build paths and environments. See [the verification notes](docs/RELEASE-VERIFICATION.md).

## Troubleshooting

- **No menu or HUD:** Confirm SFSE and SFSE Menu Framework are installed and compatible. Launch with SFSE, not the normal game executable.
- **Gauge misplaced:** Try a HUD position preset or reset position and size under Options.
- **Speed unit unexpected after update:** Existing settings take precedence over new-install defaults. Select the desired unit under Options; new installations default to KM/H.
- **Need diagnostic samples?** Enable the detailed log or CSV switches under Developer Options. These are disabled by default.

## License and credits

Starfield Vehicle Speedometer source is licensed **GPL-3.0-or-later**; see [LICENSE](LICENSE). The incorporated **SFSE Menu Framework SDK header** is MIT-licensed, with its upstream license text embedded in the header itself. See [third-party notices](THIRD_PARTY_NOTICES.md). CommonLibSF, SFSE, and SFSE Menu Framework remain their own third-party projects and have their own licenses and release requirements.

Repository: <https://github.com/VexenDeimos/Starfield-Vehicle-Speedometer>  
Author: **VexenDeimos**
