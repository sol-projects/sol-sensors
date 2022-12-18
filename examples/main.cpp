#include <sensors/sensors.hpp>
#include <sensors/Devices.hpp>

int main()
{
    auto devices = sensors::getDevices(sensors::Device::Type::CPU);
}
