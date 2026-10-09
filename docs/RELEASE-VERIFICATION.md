# v0.2.2 release verification

## Confirmed on the developer's Windows machine

- Development-workspace build: **PASS**, Windows `FileVersion=0.2.2.0`.
- In-game RC installation: **PASS** with existing `Settings.ini` preserved.
- Author reported **PASS** for the final gameplay checks (unit readouts, needle behavior, appearance and visibility settings).
- Post-test log loaded `StarfieldVehicleSpeedometer v0.2.2.0`, `metersPerWorldUnit=1.0000`, and `telemetryLog=false`. Only startup registrations appeared in the supplied log.
- Separate standalone checkout: **PASS**, `xmake build StarfieldVehicleSpeedometer`, Windows `FileVersion=0.2.2.0`, after correcting the missing speed stabilizer header in the initial staging archive.

## Recorded DLL SHA256 values

| Build | SHA256 | Usage |
| --- | --- | --- |
| Installed & gameplay-tested development-workspace DLL | `15FF35B64149E3936D2BEF22DC8F5A7AE1EC2379D22C7E424452469AD4ED2974` | **Use this exact binary for the Nexus release** |
| Independently compiled standalone DLL | `173F3A9942C6EE8DC11DB89884EA8CB50B075EB5D4DB06D7AB98817EA84F031D` | Build proof; **not yet gameplay-tested** |

Different build environments can produce different bytes; a different hash is not alone an indication of broken code. Do not substitute the untested standalone DLL into the Nexus binary package without gameplay testing it first.

## Nexus package verification

The author ran the corrected Nexus packager on Windows and reported **PASS**.

- Output: `Starfield_Vehicle_Speedometer_v0.2.2_Nexus.zip`
- Root archive entries: `SFSE/Plugins/StarfieldVehicleSpeedometer.dll`, `LICENSE.txt`, `README.txt`.
- Extracted DLL SHA256: `15FF35B64149E3936D2BEF22DC8F5A7AE1EC2379D22C7E424452469AD4ED2974`.
- The extracted binary matches the separately verified, gameplay-tested v0.2.2 DLL.
- No GitHub push or Nexus upload was performed by the packaging step.

## Calibration measurements

| In-game marker at start | In-game marker at end | Scanner delta | Plugin trip |
| ---: | ---: | ---: | ---: |
| 1838 m | 1509 m | 329 m | 329.3 U |
| 597 m | 52 m | 545 m | 547.0 U |

Both trials support an approximate 1 U = 1 m metric scale under near-straight driving conditions; they are not laboratory-precision measurements or Bethesda specifications.

## Limitations / checks not explicitly recorded

- Fresh-install behavior was checked in source and tests but not explicitly observed on a clean user profile.
- Runtime operation with **SAD fully disabled** was not separately established in the supplied test evidence, although the standalone source has no SAD dependency and builds separately.
- Compatibility across additional Starfield game builds, alternative vehicle mods, and other UI mods remains untested.
- Source carries an internal `v0.2.2-rc` startup diagnostic label; the binary's Windows FileVersion is `0.2.2.0`. Changing the compiled code solely to remove that label would require a new binary build/test cycle.

## Publication

- **No GitHub commit/push and no Nexus upload is performed by producing these archives.**
- Before release, the GPL-3.0-or-later corresponding source must be made publicly available alongside the distributed binary, and the repository/license links should be confirmed.
