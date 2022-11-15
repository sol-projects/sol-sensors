#pragma once
#include <vector>
#include "Device.hpp"

namespace sensors
{
    std::vector<Device> getDevices(Device::Type type);

    /*
    CPU: returns % Average CPU Usage across all cores
         precision: xx.x %
    RAM: returns total RAM usage in GB
         precision: xx.x GB
    GPU:

    VRAM: returns total RAM usagein GB
          precision: xx.x GB
    */
    int getLoad(const Device& device);

    /*
    CPU: returns temperature in celsius
         precision: xx C
    RAM: returns 0
    GPU: returns temperature in celsius
         precision: xx C
    VRAM: returns temperature in celsius
         precision: xx C
    */
    int getTemp(const Device& device);
}

