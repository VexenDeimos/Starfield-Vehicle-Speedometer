#pragma once
// Independent visual colors for the standalone HUD. Pure C++ (no SFSE dependencies).
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace telemetry::palette
{
    using Rgb = std::array<float,3>;
    struct Style {
        Rgb needle{};
        Rgb number{};
        Rgb units{};
        Rgb rim{};
        Rgb ticks{};
        Rgb background{};
        Rgb progress{};
    };

    // 0 original Starfield blue; 1 white; 2 amber; 3 custom (no preset overwrite).
    inline constexpr Style kBlue {
        {148.0f/255.0f,212.0f/255.0f,246.0f/255.0f}, // #94D4F6
        {1.0f,1.0f,1.0f},
        {225.0f/255.0f,227.0f/255.0f,228.0f/255.0f},
        {222.0f/255.0f,224.0f/255.0f,225.0f/255.0f},
        {225.0f/255.0f,227.0f/255.0f,228.0f/255.0f},
        {39.0f/255.0f,41.0f/255.0f,43.0f/255.0f},
        {148.0f/255.0f,212.0f/255.0f,246.0f/255.0f}
    };
    inline constexpr Style kWhite {
        {242.0f/255.0f,238.0f/255.0f,238.0f/255.0f},
        {1.0f,1.0f,1.0f},
        {225.0f/255.0f,227.0f/255.0f,228.0f/255.0f},
        {222.0f/255.0f,224.0f/255.0f,225.0f/255.0f},
        {225.0f/255.0f,227.0f/255.0f,228.0f/255.0f},
        {39.0f/255.0f,41.0f/255.0f,43.0f/255.0f},
        {242.0f/255.0f,238.0f/255.0f,238.0f/255.0f}
    };
    inline constexpr Style kAmber {
        {1.0f,0.70f,0.29f}, {1.0f,0.96f,0.87f},
        {0.94f,0.82f,0.67f}, {0.92f,0.72f,0.43f},
        {0.90f,0.80f,0.68f}, {0.15f,0.125f,0.105f},
        {1.0f,0.70f,0.29f}
    };

    inline constexpr Style preset(int id) noexcept {
        return id == 1 ? kWhite : id == 2 ? kAmber : kBlue;
    }
    inline float bounded(float value) noexcept {
        return std::isfinite(value) ? std::clamp(value,0.0f,1.0f) : 0.0f;
    }
    inline std::uint8_t byte(float value) noexcept {
        return static_cast<std::uint8_t>(std::lround(bounded(value)*255.0f));
    }
    // ImGui drawing API uses ABGR hexadecimal layout, not RGBA.
    inline std::uint32_t abgr(const Rgb& rgb,float alpha=1.0f) noexcept {
        return (static_cast<std::uint32_t>(byte(alpha)) << 24) |
               (static_cast<std::uint32_t>(byte(rgb[2])) << 16) |
               (static_cast<std::uint32_t>(byte(rgb[1])) << 8) |
               static_cast<std::uint32_t>(byte(rgb[0]));
    }
}
