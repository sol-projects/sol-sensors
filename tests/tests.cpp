#include "sensors/sensors.hpp"
#include <cassert>
#include <LLOG/llog.hpp>

int main()
{
    auto devices = sensors::getDevices(sensors::Device::Type::Any);

    assert(!devices.empty());

    llog::Print("Number of devices:", std::size(devices));
    llog::Print(devices);

    llog::Print("CPU temperature:", sensors::getTemp(devices.at(0)));
    llog::Print("GPU temperature:", sensors::getTemp(devices.at(1)));
    llog::Print("RAM usage:", sensors::getLoad(devices.at(2)));
    return 0;
}
