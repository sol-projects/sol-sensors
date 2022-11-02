#include "sensors/sensors.hpp"
#include <cassert>
#include <LLOG/llog.hpp>

int main()
{
    auto devices = sensors::getDevices(sensors::Device::Type::Any);

    assert(!devices.empty());

    llog::Print(devices);

    return 0;
}
