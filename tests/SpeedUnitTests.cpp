#include "Telemetry/SpeedUnits.h"
#include "Telemetry/GaugeLayout.h"
#include <cmath>
#include <cassert>
#include <limits>
#include <cstring>
#include <cstdio>
static bool near(float a,float b,float tol=.02f){return std::fabs(a-b)<tol;}
int main(){
 using namespace telemetry::speedunits;
 assert(near(convert(15,kRaw,1),15));
 assert(near(convert(15,kMetersPerSecond,1),15));
 assert(near(convert(15,kKmh,1),54));
 assert(near(convert(15,kMph,1),33.554));
 assert(near(convert(15,kKmh,0.5),27));
 assert(near(convert(15,kMph,0.5),16.777));
 assert(near(convert(35,kKmh,1),126));
 assert(near(convert(15,kKmh,1)/convert(35,kKmh,1),15.f/35));
 assert(near(convert(15,kMph,1)/convert(35,kMph,1),15.f/35));
 assert(near(convert(15,kRaw,1)/convert(35,kRaw,1),15.f/35));
 assert(near(convert(-5,kKmh,1),0));
 assert(near(convert(std::numeric_limits<float>::quiet_NaN(),kKmh,1),0));
 assert(near(safeMetersPerWorldUnit(std::numeric_limits<float>::infinity()),1));
 assert(near(safeMetersPerWorldUnit(-6),.01));
 assert(near(safeMetersPerWorldUnit(100),5));
 assert(std::strcmp(label(kRaw),"u/s")==0);
 assert(std::strcmp(label(kKmh),"km/h")==0);
 assert(std::strcmp(label(kMph),"mph")==0);
 assert(std::strcmp(label(kMetersPerSecond),"m/s")==0);
 // Pure layout / smoothing regressions; changing units must not affect needle ratio.
 assert(near(telemetry::gauge::dialFraction(15,35),15.f/35));
 const auto center=telemetry::gauge::resolve(1920,1080,1.218,3,320,700,217.630f,-73.185f);
 assert(std::isfinite(center.x)&&std::isfinite(center.y));
 telemetry::gauge::NeedleSmoother needle;
 assert(near(needle.observe(15,0.05f,.8f),15));
 assert(needle.observe(16,.05f,.8f)>15);
 std::puts("PASS: conversions, labels, dial invariance, bounded scale, needle/layout regressions");
}
