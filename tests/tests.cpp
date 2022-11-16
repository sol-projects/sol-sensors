#include "sensors/sensors.hpp"
#include <cassert>
#include <LLOG/llog.hpp>
#include <chrono>
#include <thread>

int main()
{
    using namespace std::chrono_literals;

    for(auto& device : sensors::getDevices(sensors::Device::Type::Any))
    {
        switch(device.type)
        {
            case sensors::Device::Type::CPU:
            {
                llog::Print("Device name:", device.name);
                llog::Print("-Temperature:", sensors::getTemp(device));
                std::this_thread::sleep_for(200ms);
                auto loadf = std::to_string(sensors::getLoad(device));
                llog::Print("-Load: ", std::string(loadf.substr(0, loadf.size()-1) + "." + loadf.back() + "%"));
                break;
            }

            case sensors::Device::Type::RAM:
            {
                llog::Print("Device name:", device.name);
                llog::Print("-Temperature:", sensors::getTemp(device));
                auto loadf = std::to_string(sensors::getLoad(device));
                llog::Print("-Load:", std::string(loadf.substr(0, loadf.size()-1) + "." + loadf.back() + " GB"));
                break;
            }

            case sensors::Device::Type::GPU:
            {
                llog::Print("Device name:", device.name);
                llog::Print("-Temperature:", sensors::getTemp(device));
                llog::Print("-Load:", std::to_string(sensors::getLoad(device)) + "%");
                break;
            }

            case sensors::Device::Type::VRAM:
            {
                llog::Print("Device name:", device.name);
                llog::Print("-Temperature:", sensors::getTemp(device));
                auto loadf = std::to_string(sensors::getLoad(device));
                llog::Print("-Load:", std::string(loadf.substr(0, loadf.size()-1) + "." + loadf.back() + " GB"));
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
