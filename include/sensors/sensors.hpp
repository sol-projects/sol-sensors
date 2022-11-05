#pragma once
#include <vector>
#include "Device.hpp"

namespace sensors
{
    std::vector<Device> getDevices(Device::Type type);
    int getLoad(const Device& device);
    int getTemp(const Device& device);
}

