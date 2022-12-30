#pragma once
#include <iostream>
#include <string>

namespace sensors
{
    struct Device
    {
        enum class Type
        {
            CPU,
            CPUThread,
            GPU,
            RAM,
            VRAM,
            Any
        };

        static int generateID()
        {
            static int i;
            return i++;
        }

        int id;
        std::string name;
        Type type;
        int temperature;
        int load;
    };

    inline std::ostream& operator<<(std::ostream& os, const Device& device)
    {
        os << device.id << " " << device.name << " " << device.temperature << " " << device.load;
        return os;
    }

}
