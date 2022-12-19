#include <sensors/sensors.hpp>
#include <sensors/Device.hpp>
#include <iostream>
#include <sensors/error.hpp>

int main()
{
    auto devices = sensors::getDevices(sensors::Device::Type::RAM);

    for(auto& device : devices)
    {
        device.load = sensors::getLoad(device);
        device.temperature = sensors::getTemp(device);

        if(device.load == sensors::error::code)
        {
            std::cout << "Cannot get load!" << '\n';
        }
        else
        {
            std::cout << device.load << '\n';
        }

        if(device.temperature == sensors::error::code)
        {
            std::cout << "Cannot get temperature!" << '\n';
        }
        else
        {
            std::cout << device.temperature << '\n';
        }
    }
}

