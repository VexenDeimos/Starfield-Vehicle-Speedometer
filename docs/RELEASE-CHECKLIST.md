# Publication checklist — v0.2.2

## Completed

- [x] User accepted **GPL-3.0-or-later**.
- [x] v0.2.2 source built in the development workspace.
- [x] Installed release candidate (`FileVersion=0.2.2.0`) and verified gameplay.
- [x] Persisted HUD preferences, units, appearance, and visibility worked in the author's final gameplay session.
- [x] Post-test log showed `telemetryLog=false` and minimal startup messages.
- [x] Standalone Windows source build passed after restoring missing `VehicleSpeedStabilizer` code.
- [x] Two in-game distance tests support a 1 m/world unit default.

## Before publishing

- [x] Build Nexus ZIP **from the exact in-game-tested installed DLL** using the checksum-protecting packager.
- [x] Review ZIP content and extracted DLL hash; source and binary both report v0.2.2.
- [ ] Make this corresponding source publicly accessible (GitHub repository + release tag recommended).
- [ ] Verify the GitHub repository's README, LICENSE, changelog, and third-party notices render as expected.
- [ ] Check version-specific compatibility requirements for installed Starfield, SFSE, and SFSE Menu Framework.
- [ ] Choose/upload Nexus screenshots and review the Nexus listing/requirements.
- [ ] Explicitly authorize the GitHub push, tagging, and Nexus publication.

## Optional broader tests

- [ ] Test on a fresh `Settings.ini` with KM/H default, separate from existing player preferences.
- [ ] Test gameplay with SAD disabled and with unrelated SFSE plugins removed.
- [ ] Test additional game versions and third-party HUD/vehicle modifications as applicable.
