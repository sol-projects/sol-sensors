#include "sensors/sensors.hpp"
#include <cassert>
#include <LLOG/llog.hpp>

int main()
{

    for(const auto& device : sensors::getDevices(sensors::Device::Type::Any))
    {
        switch(device.type)
        {
            case sensors::Device::Type::CPU:
            {
                llog::Print("Device name:", device.name);
                llog::Print("-Temperature:", sensors::getTemp(device));
                llog::Print("-Load: ", sensors::getLoad(device));
                break;
            }

            case sensors::Device::Type::RAM:
            {
                llog::Print("Device name:", device.name);
                llog::Print("-Temperature:", sensors::getTemp(device));
                llog::Print("-Load:", sensors::getLoad(device));
                break;
            }

            case sensors::Device::Type::GPU:
            {
                llog::Print("Device name:", device.name);
                llog::Print("-Temperature:", sensors::getTemp(device));
                llog::Print("-Load:", sensors::getLoad(device));
                break;
            }

            case sensors::Device::Type::Any:
            {
                llog::Print(llog::pt::error, "Unknown device type", device.name);
            }
        }
    }

    return 0;
}
