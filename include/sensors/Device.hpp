#pragma once
#include <string>

namespace sensors
{
    struct Device
    {
        enum class Type
        {
            CPU, GPU
        };

        std::string name;
        Type type;
    };
}
