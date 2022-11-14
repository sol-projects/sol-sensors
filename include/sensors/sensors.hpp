#pragma once
#include <vector>
#include "Device.hpp"

namespace sensors
{
    std::vector<Device> getDevices(Device::Type type);

    //CPU: returns % Average CPU Usage across all cores
    //     precision: xx.x %
    //RAM: returns total RAM usage in GB
    //     precision: xx.x GB
    //GPU:
    //
    int getLoad(const Device& device);

    //CPU: returns temperature
    //     precision: xx C
    //RAM: returns 0
    //
    //GPU: returns temperature in celsius
    //     precision: xx C
    int getTemp(const Device& device);
}

