#pragma once
#include "Device.hpp"
#include <vector>

namespace sensors
{
    std::vector<Device> getDevices(Device::Type type);
    int getLoad(const Device& device, int precision = 0);
    int getTemp(const Device& device, int precision = 0);
}
