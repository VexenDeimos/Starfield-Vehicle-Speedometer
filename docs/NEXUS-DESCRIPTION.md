# Nexus Mods listing draft — Starfield Vehicle Speedometer v0.2.2

**Suggested title:** Starfield Vehicle Speedometer  
**Version:** 0.2.2  
**Author:** VexenDeimos  
**Category:** Gameplay / User Interface (select the available Nexus category)  
**License:** GPL-3.0-or-later for the mod source (SFSE Menu Framework header remains MIT-licensed)

## Short description

A fully customizable circular speedometer for Starfield land vehicles, featuring MPH, KM/H, M/S, and U/S readouts, customizable colors, background transparency, adjustable positioning, and smooth needle movement.

I originally created this as a tool to help test my **Starfield Advanced DualSense** mod. After seeing how useful it was, I decided to polish it up, add more customization options, and share it with the community.

## About this mod

Add a clean, compact vehicle speedometer to Starfield without replacing your entire HUD. The gauge tracks land-vehicle movement, smoothly animates acceleration and braking, and displays your speed in your choice of kilometers per hour, miles per hour, meters per second, or raw game units per second.

Customize the dial to match your HUD with Starfield Blue, Starfield White, Amber, or fully custom colors. Adjust the needle, numbers, labels, rim, ticks, progress arc, and background independently, including a fully transparent gauge background. Choose a preset HUD location or tune the size and offsets yourself.

The default metric scale (1 world unit = 1 meter) was checked in-game against two straight-line scanner-distance tests. Advanced users can also adjust the scale and inspect telemetry.

### Highlights

- Compact circular speedometer, no full-HUD replacement.
- KM/H, MPH, M/S, U/S with automatic needle/dial scaling.
- Smooth needle movement and stable numerical readout.
- Customizable colors and 0–100% background opacity.
- Position presets, pixel offsets, gauge sizing, and persistent settings.
- Optional vehicle-exit idle HUD, in-menu preview, peak speed, and diagnostic logging.
- No SAD or Map Cursor Meter dependency; standalone SFSE plugin.

### Requirements

- Starfield Script Extender (SFSE)
- Address Library for SFSE Plugins
- SFSE Menu Framework

### Install

Install with **Vortex** or **Mod Organizer 2** using the provided archive. For manual installation, place the DLL at `Starfield/Data/SFSE/Plugins/StarfieldVehicleSpeedometer.dll` and launch through SFSE.

Open **F1 → Vehicle Speedometer** to configure the HUD. Updates preserve saved settings.

### Updating / Removing

Replace or remove `StarfieldVehicleSpeedometer.dll` under `Data/SFSE/Plugins`. Remove `Data/SFSE/Plugins/VehicleSpeedometer/` only if you want to delete this mod's saved settings and CSV files.

### Compatibility and calibration

Requires SFSE Menu Framework to display its menu and HUD. Starfield Advanced DualSense and controller wrappers are **not required**. The metric conversion was empirically checked against the in-game scanner, not sourced from an official Bethesda conversion specification. Not tested on every game build or with every vehicle/HUD mod.

### Source and license

Complete corresponding source: <https://github.com/VexenDeimos/Starfield-Vehicle-Speedometer> (link will work as a source release after publication).  
License: GPL-3.0-or-later. Upstream SFSE Menu Framework SDK header: MIT license, notices preserved in the source repository.

### Changelog v0.2.2

- Field-checked metric conversion, removed approximate unit marker.
- Quieter logging by default, optional verbose telemetry retained.
- Refined smooth needle response and anti-flicker readout.
- Custom colors, transparency, HUD positioning, and saved options.
