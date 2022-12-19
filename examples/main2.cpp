#include <sensors/sensors.hpp>
#include <sensors/Device.hpp>
#include <iostream>

int main()
{
    auto devices = sensors::getDevices(sensors::Device::Type::CPU);

    for(auto& device : devices)
    {
        device.load = sensors::getLoad(device);
        device.temperature = sensors::getTemp(device);
        std::cout << device.load << '\n';
        std::cout << device.temperature << '\n';
    }
}

