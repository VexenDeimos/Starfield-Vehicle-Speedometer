from pathlib import Path
s=Path(__file__).resolve().parents[1].joinpath('src/VehicleSpeedometer.cpp').read_text(encoding='utf-8')
u=Path(__file__).resolve().parents[1].joinpath('include/Telemetry/SpeedUnits.h').read_text(encoding='utf-8')
assert 'g_metricCalibrationVerified' not in s
assert '(g_speedUnits!=0 &&' not in s
assert 'metricCalibrationVerified") { /* Legacy' in s
assert 'Write detailed telemetry to log' in s
assert 'g_diagnosticLogging &&' in s
assert 'std::snprintf(line,sizeof(line),"%d",g_displayedSpeed)' in s
assert 'field-checked' in s.lower()
assert 'kDefaultMetersPerWorldUnit' in u
assert 'kProvisionalMetersPerWorldUnit' not in u
print('PASS: v0.2.2 release cleanup regression')
