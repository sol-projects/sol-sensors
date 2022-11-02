#pragma once
#include <string>
#include <iostream>

namespace sensors
{
    struct Device
    {
        enum class Type
        {
            CPU, GPU, RAM, Any
        };

        std::string name;
        Type type;
        int temperature;
        int load;
    };

    inline std::ostream& operator<<(std::ostream& os, const Device& device)
    {
        os << device.name << " " << device.temperature << " " << device.load;
        return os;
    }

}
