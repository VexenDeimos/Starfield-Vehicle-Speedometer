#include <Telemetry/SampleMath.h>
#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    telemetry::CursorMeter c;
    assert(!c.observe({100, 100}, 10.0, true).valid);
    auto cm = c.observe({160, 180}, 11.0, true);
    assert(!cm.valid); // 1-second sampling gap invalid by design
    assert(!c.observe({160, 180}, 11.1, false).valid);
    assert(!c.observe({10, 10}, 11.5, true).valid);
    cm = c.observe({16, 18}, 11.6, true);
    assert(cm.valid && std::abs(cm.speedPixelsPerSecond - 100.0) < 1e-6);

    telemetry::VehicleMeter v;
    assert(!v.observe(1, {0,0,0}, 1.0).valid);
    auto vm = v.observe(1, {3,4,0}, 1.1);
    assert(vm.valid && std::abs(vm.horizontalUnitsPerSecond - 50.0) < 1e-6);
    vm = v.observe(1, {6,8,6}, 1.2);
    assert(vm.valid && std::abs(vm.horizontalUnitsPerSecond - 50.0) < 1e-6);
    assert(std::abs(vm.threeDimensionalUnitsPerSecond - std::hypot(50.0,60.0)) < 1e-6);
    assert(!v.observe(2, {0,0,0}, 1.3).valid); // new vehicle: no bogus jump
    vm = v.observe(2, {100,0,0}, 1.4);
    assert(vm.valid);
    assert(!v.observe(2, {200,0,0}, 3.0).valid); // stale samples
    assert(!v.observe(0, {0,0,0}, 3.1).valid); // exited vehicle
    assert(!v.observe(2, {0,0,0}, 3.2).valid);
    assert(!v.observe(2, {1e9,0,0}, 3.3).valid); // teleport rejection
    std::cout << "PASS cursor measurement: real delta/time, gaps, reset\n";
    std::cout << "PASS vehicle measurement: 2D/3D/vertical, invalid state, identity, teleport\n";
}
