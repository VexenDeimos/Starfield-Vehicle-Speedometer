#pragma once
// Pure, UI-independent geometry for the standalone REV-8 gauge.
// Positions are HUD pixel coordinates with origin at the top-left.
#include <algorithm>
#include <cmath>

namespace telemetry::gauge
{
    // Recorded calibration from the user's preferred beside-compass placement.
    inline constexpr float kCompassXOffset = -15.407f;
    inline constexpr float kCompassYOffset = 42.371f;
    inline constexpr float kCompassSize = 1.218f;
    // Recorded from the tester's chosen Above Health HUD placement.
    inline constexpr float kHealthXOffset = 217.630f;
    inline constexpr float kHealthYOffset = -73.185f;
    inline constexpr float kHealthSize = 1.218f;
    // Display-only dial response. The user-set responseSeconds controls
    // cruising stability, while sustained acceleration and braking shorten
    // the time constant to keep the needle and number responsive.
    // Raw velocity, peaks, calibration trips and CSV are never affected.
    struct NeedleSmoother
    {
        float displayed{0.0f};
        float previousTarget{0.0f};
        float trendDuration{0.0f};
        int trendDirection{0};
        bool initialized{false};

        void reset() noexcept
        {
            displayed=0.0f; previousTarget=0.0f;
            trendDuration=0.0f; trendDirection=0; initialized=false;
        }

        float observe(float actual, float deltaSeconds, float responseSeconds) noexcept
        {
            if (!std::isfinite(actual)) actual=0.0f;
            actual=std::clamp(actual,0.0f,9999.0f);
            if (!initialized || !std::isfinite(displayed)) {
                initialized=true;
                displayed=actual;
                previousTarget=actual;
                return displayed;
            }
            if (!std::isfinite(responseSeconds) || responseSeconds<=0.01f) {
                displayed=actual;
                previousTarget=actual;
                trendDuration=0.0f; trendDirection=0;
                return displayed;
            }
            const float dt=std::clamp(std::isfinite(deltaSeconds)?deltaSeconds:0.0f,0.0f,0.15f);
            const float cruiseTau=std::clamp(responseSeconds,0.05f,2.0f);
            const float error=actual-displayed;

            // Repeated changes in the same direction suggest real vehicle
            // acceleration. Alternating frame/physics jitter stays on the
            // stronger cruise smoothing instead of jerking the pointer.
            const float sourceDelta=actual-previousTarget;
            const int direction=error>1.4f ? 1 : (error < -1.4f ? -1 : 0);
            if (direction!=0 && direction==trendDirection &&
                sourceDelta*static_cast<float>(direction)>=-0.20f) {
                trendDuration=std::min(trendDuration+dt,1.0f);
            } else {
                trendDuration=0.0f;
            }
            trendDirection=direction;
            previousTarget=actual;

            float tau=cruiseTau;
            if (displayed<1.0f && actual>1.2f) {
                tau=std::min(cruiseTau,0.16f); // prompt start from rest
            } else if (actual<=0.4f) {
                tau=std::min(cruiseTau,0.24f); // smooth decay to rest
            } else if (trendDuration>=0.10f && direction!=0) {
                tau=std::min(cruiseTau,0.22f); // sustained acceleration/braking
            }
            // A small hold band eliminates trembling around the same speed.
            // Never apply it to standstill or significant deceleration.
            if (actual>0.4f && std::fabs(error)<0.22f) return displayed;
            displayed += (1.0f-std::exp(-dt/tau))*error;
            if (actual<=0.4f && displayed<0.10f) displayed=0.0f;
            return displayed;
        }
    };

    // Display readout hysteresis is expressed in RAW world-speed units,
    // then converted. Without that, 1 U/S of normal sensor jitter becomes
    // 3.6 km/h and flips the number constantly. Only the HUD is affected.
    inline int steadyIntegerSpeedScaled(float converted, int previouslyShown,
        float unitFactor) noexcept
    {
        if (!std::isfinite(converted) || converted<=0.0f) return 0;
        const float speed=std::clamp(converted,0.0f,99999.0f);
        const int candidate=static_cast<int>(std::lround(speed));
        if (previouslyShown<0) return candidate;
        const float factor=std::isfinite(unitFactor) ? std::clamp(unitFactor,0.01f,18.0f) : 1.0f;
        // About 0.65 raw U/S for all units; at least 0.85 displayed units.
        const float band=std::max(0.85f,0.65f*factor);
        if (std::fabs(speed-static_cast<float>(previouslyShown))<band) return previouslyShown;
        return candidate;
    }

    // Keep the legacy raw readout helper for source regressions / tests.
    inline int steadyIntegerSpeed(float speed, int previouslyShown) noexcept
    {
        return steadyIntegerSpeedScaled(speed,previouslyShown,1.0f);
    }

    struct Center { float x{0.0f}; float y{0.0f}; };

    inline float safeScale(float scale) noexcept
    {
        return std::isfinite(scale) ? std::clamp(scale,0.60f,1.80f) : 0.88f;
    }

    inline float dialFraction(float speed, float fullScale) noexcept
    {
        if (!std::isfinite(speed) || !std::isfinite(fullScale) || fullScale < 10.0f)
            return 0.0f;
        return std::clamp(speed / fullScale,0.0f,1.0f);
    }

    // preset: 0 compass, 1 top-left, 2 top-right,
    // 3 above health HUD, 4 bottom-right, 5 custom center.
    inline Center resolve(float width,float height,float dialScale,int preset,
        float customX,float customY,float offsetX,float offsetY) noexcept
    {
        const float scale=safeScale(dialScale);
        const float radius=72.0f*scale;
        const float margin=16.0f;
        const float w = std::isfinite(width) ? std::max(1.0f,width) : 1920.0f;
        const float h = std::isfinite(height) ? std::max(1.0f,height) : 1080.0f;
        const float left=radius+margin,right=std::max(left,w-radius-margin);
        const float top=radius+margin,bottom=std::max(top,h-radius-margin);
        Center p{};
        switch(preset) {
            case 1: p={left,top}; break;
            case 2: p={right,top}; break;
            case 3: p={w*0.76f,h-176.0f*scale}; break;
            case 4: p={right,bottom}; break;
            case 5: p={customX,customY}; break;
            default: p={356.0f*scale,h-150.0f*scale}; break;
        }
        const float dx=std::isfinite(offsetX)?offsetX:0.0f;
        const float dy=std::isfinite(offsetY)?offsetY:0.0f;
        const float px=std::isfinite(p.x)?p.x:left;
        const float py=std::isfinite(p.y)?p.y:top;
        return {std::clamp(px+dx,left,right),std::clamp(py+dy,top,bottom)};
    }
}
