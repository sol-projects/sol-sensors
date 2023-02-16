#include "sensors/sensors.hpp"
#include "sensors/error.hpp"

#ifdef _WIN32

namespace sensors
{
    std::vector<Device> getDevices(Device::Type type)
    {
        return {};
    }

    int getLoad(const Device& device, int precision)
    {
        return error::code;
    }

    int getTemp(const Device& device, int precision)
    {
        return error::code;
    }
}
#endif
