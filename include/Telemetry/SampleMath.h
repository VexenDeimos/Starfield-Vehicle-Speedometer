#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace telemetry
{
    struct Vector2 { double x{0.0}; double y{0.0}; };
    struct Vector3 { double x{0.0}; double y{0.0}; double z{0.0}; };

    inline double magnitude(Vector2 v) noexcept
    {
        return std::hypot(v.x, v.y);
    }

    struct CursorSample
    {
        bool valid{false};
        double speedPixelsPerSecond{0.0};
        double deltaPixels{0.0};
    };

    // Screen coordinates must come from a verified game-cursor source to
    // represent actual map-cursor speed. ImGui mouse position is EXPERIMENTAL.
    class CursorMeter
    {
    public:
        CursorSample observe(Vector2 pixel, double timeSeconds, bool sourceValid)
        {
            CursorSample out{};
            if (!sourceValid || !std::isfinite(timeSeconds) ||
                !std::isfinite(pixel.x) || !std::isfinite(pixel.y)) {
                reset();
                return out;
            }
            if (_have && timeSeconds > _time) {
                const double dt = timeSeconds - _time;
                if (dt >= 0.001 && dt <= 0.25) {
                    const double dx = pixel.x - _pos.x;
                    const double dy = pixel.y - _pos.y;
                    const double dist = std::hypot(dx, dy);
                    if (std::isfinite(dist)) {
                        out.valid = true;
                        out.deltaPixels = dist;
                        out.speedPixelsPerSecond = dist / dt;
                    }
                }
            }
            _pos = pixel;
            _time = timeSeconds;
            _have = true;
            return out;
        }

        void reset() noexcept { _have = false; _pos = {}; _time = 0.0; }
    private:
        bool _have{false};
        Vector2 _pos{};
        double _time{0.0};
    };

    struct SpeedSample
    {
        bool valid{false};
        double horizontalUnitsPerSecond{0.0};
        double threeDimensionalUnitsPerSecond{0.0};
        double verticalUnitsPerSecond{0.0};
        double accelerationUnitsPerSecondSquared{0.0};
        bool accelerationValid{false};
    };

    // Filters telemetry sample gaps and isolated zero-displacement frames.
    // Time-based, so 20 Hz gameplay polling does not cause frame-rate bias.
    struct StabilizedSpeed
    {
        bool valid{false};
        double speed{0.0};
        bool held{false};
    };

    class VehicleSpeedStabilizer
    {
    public:
        StabilizedSpeed observe(bool sampleValid, double rawSpeed, double now) noexcept
        {
            if (!std::isfinite(now)) { reset(); return {}; }
            if (sampleValid && std::isfinite(rawSpeed) && rawSpeed >= 0.0 && rawSpeed <= 100000.0) {
                // A single stationary reference read during movement should
                // not yank the needle to zero. A real stop is accepted after
                // 0.22s, then eased down naturally.
                if (_have && rawSpeed < 3.0 && _filtered > 8.0 &&
                    now - _lastMoving < 0.22) {
                    _lastSample = now;
                    return {true, _filtered, true};
                }
                const double dt = _have ? std::clamp(now - _lastSample, 0.005, 0.25) : 0.0;
                const double alpha = _have ? 1.0 - std::exp(-dt / 0.20) : 1.0;
                _filtered = _have ? _filtered + alpha * (rawSpeed - _filtered) : rawSpeed;
                _have = true;
                _lastSample = now;
                if (rawSpeed >= 3.0) _lastMoving = now;
                return {true, _filtered, false};
            }
            // Short bad samples hold the last real value, then expire rather
            // than displaying fabricated motion forever.
            if (_have && now >= _lastSample && now - _lastSample <= 0.60)
                return {true, _filtered, true};
            return {};
        }

        void reset() noexcept
        {
            _have = false;
            _filtered = 0.0;
            _lastSample = 0.0;
            _lastMoving = -1e30;
        }

    private:
        bool _have{false};
        double _filtered{0.0};
        double _lastSample{0.0};
        double _lastMoving{-1e30};
    };

    class VehicleMeter
    {
    public:
        SpeedSample observe(std::uintptr_t vehicleIdentity, Vector3 p, double timestampSeconds)
        {
            SpeedSample out{};
            if (vehicleIdentity == 0 || !std::isfinite(timestampSeconds) ||
                !std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) {
                reset();
                return out;
            }
            if (_have && vehicleIdentity == _identity && timestampSeconds > _time) {
                const double dt = timestampSeconds - _time;
                if (dt >= 0.005 && dt <= 1.0) {
                    const double vx = (p.x - _pos.x) / dt;
                    const double vy = (p.y - _pos.y) / dt;
                    const double vz = (p.z - _pos.z) / dt;
                    const double horizontal = std::hypot(vx, vy);
                    const double speed3d = std::hypot(horizontal, vz);
                    // Teleports and invalid physics states must never generate
                    // spectacular but misleading speedometer readings.
                    if (std::isfinite(speed3d) && speed3d <= 100000.0) {
                        out.valid = true;
                        out.horizontalUnitsPerSecond = horizontal;
                        out.threeDimensionalUnitsPerSecond = speed3d;
                        out.verticalUnitsPerSecond = vz;
                        if (_previousSpeedValid) {
                            out.accelerationValid = true;
                            out.accelerationUnitsPerSecondSquared = (horizontal - _previousSpeed) / dt;
                        }
                        _previousSpeed = horizontal;
                        _previousSpeedValid = true;
                    } else {
                        _previousSpeedValid = false;
                    }
                } else {
                    _previousSpeedValid = false;
                }
            } else {
                _previousSpeedValid = false;
            }
            _pos = p;
            _time = timestampSeconds;
            _have = true;
            _identity = vehicleIdentity;
            return out;
        }
        void reset() noexcept
        {
            _have = false;
            _pos = {};
            _time = 0.0;
            _identity = 0;
            _previousSpeed = 0.0;
            _previousSpeedValid = false;
        }
    private:
        bool _have{false};
        Vector3 _pos{};
        double _time{0.0};
        std::uintptr_t _identity{0};
        double _previousSpeed{0.0};
        bool _previousSpeedValid{false};
    };
}
