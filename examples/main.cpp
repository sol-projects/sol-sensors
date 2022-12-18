#include <sensors/sensors.hpp>
#include <sensors/Device.hpp>
#include <iostream>

int main()
{
    auto devices = sensors::getDevices(sensors::Device::Type::Any);

    for(auto& device : devices)
    {
        std::cout << device.name << '\n';
    }
}
