#include "sensors/sensors.hpp"

#ifdef _WIN32

namespace
{
    //declare variables you can use in this file
}

namespace sensors
{
    std::vector<Device> getDevices(Device::Type type)
    {
        return {};
    }
}
#endif
