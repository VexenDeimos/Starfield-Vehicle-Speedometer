#include <Telemetry/SampleMath.h>
#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    telemetry::VehicleSpeedStabilizer filter;
    auto cruise = filter.observe(true, 15.0, 1.0);
    assert(cruise.valid && !cruise.held && std::fabs(cruise.speed - 15.0) < 1e-6);
    auto isolatedZero = filter.observe(true, 0.0, 1.05);
    assert(isolatedZero.valid && isolatedZero.held && std::fabs(isolatedZero.speed - 15.0) < 1e-6);
    auto realStop = filter.observe(true, 0.0, 1.30);
    assert(realStop.valid && !realStop.held && realStop.speed < 15.0);
    auto shortGap = filter.observe(false, 0.0, 1.40);
    assert(shortGap.valid && shortGap.held);
    auto expired = filter.observe(false, 0.0, 2.20);
    assert(!expired.valid);
    filter.reset();
    auto restart = filter.observe(true, 8.0, 3.0);
    assert(restart.valid && std::fabs(restart.speed - 8.0) < 1e-6);
    std::cout << "PASS: vehicle stabilization, isolated-zero hold, expiry, reset\n";
}
