#include "sensors/sensors.hpp"
#include <cassert>
#include <LLOG/llog.hpp>

int main()
{
    auto devices = sensors::getDevices(sensors::Device::Type::Any);

    assert(!devices.empty());

    llog::Print("Number of devices:", std::size(devices));
    llog::Print(devices);

    assert(sensors::getTemp(devices.at(0)) != 0);
    llog::Print("CPU temperature:", sensors::getTemp(devices.at(0)));
    return 0;
}
