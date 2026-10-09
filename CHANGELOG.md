# Changelog

## v0.2.2 — First public-release candidate

**Verified by the author in Starfield; independent Windows source build passed. Public publication still requires an explicit release decision.**

- Field-validated metric default of **1 world unit = 1 meter** with two in-game REV-8 scanner-distance tests.
- Remove the provisional `~` speed-unit marker and obsolete manual metric-verification checkbox.
- Preserve previous settings and default new installs to KM/H; MPH, M/S, and raw U/S remain available.
- Disable chatty telemetry logging by default while retaining opt-in diagnostic logs and CSV capture.
- Update Windows DLL file version to `0.2.2.0`.
- Keep HUD colors, transparency, position presets, menu visibility, and improved needle response unchanged.
- Correct standalone source packaging and verify a clean Windows standalone build.

## v0.2.1 — Development

- Synchronize the speed readout with needle movement and improve the transition to zero.
- Reduce unwanted numeric flicker with unit-aware readout stabilization.

## v0.2.0 — Development

- Add KM/H, MPH, M/S, and raw U/S with unit-scaled dial markings.
- Add trip-distance calibration and an adjustable world-unit conversion.

## v0.1.x — Development

- Create the circular HUD and vehicle-position-based speed telemetry.
- Add dial positioning presets, scaling, color themes, transparency, saved settings, and developer diagnostics.
