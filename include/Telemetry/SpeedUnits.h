#pragma once
// Pure display conversion for the standalone land-vehicle speedometer.
// Scanner-distance checks in Starfield support the default 1 world unit = 1 metre.
// This is an in-game field check, not a Bethesda-published engine specification.
#include <algorithm>
#include <cmath>

namespace telemetry::speedunits
{
    // Persistent IDs: 0 raw u/s, 1 km/h, 2 mph, 3 m/s.
    inline constexpr int kRaw = 0;
    inline constexpr int kKmh = 1;
    inline constexpr int kMph = 2;
    inline constexpr int kMetersPerSecond = 3;
    inline constexpr float kDefaultMetersPerWorldUnit = 1.0f;

    inline float safeMetersPerWorldUnit(float scale) noexcept
    {
        return std::isfinite(scale) ? std::clamp(scale,0.01f,5.0f) : kDefaultMetersPerWorldUnit;
    }

    inline float factor(int units, float metersPerWorldUnit) noexcept
    {
        const float m=safeMetersPerWorldUnit(metersPerWorldUnit);
        switch(units) {
            case kKmh: return m*3.6f;
            case kMph: return m*2.2369362921f;
            case kMetersPerSecond: return m;
            default: return 1.0f;
        }
    }

    inline float convert(float rawWorldUnitsPerSecond,int units,float metersPerWorldUnit) noexcept
    {
        if (!std::isfinite(rawWorldUnitsPerSecond)) return 0.0f;
        return std::max(0.0f,rawWorldUnitsPerSecond)*factor(units,metersPerWorldUnit);
    }

    inline const char* label(int units) noexcept
    {
        switch(units) {
            case kKmh: return "km/h";
            case kMph: return "mph";
            case kMetersPerSecond: return "m/s";
            default: return "u/s";
        }
    }
}
