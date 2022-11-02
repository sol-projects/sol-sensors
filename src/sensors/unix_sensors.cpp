#include "sensors/sensors.hpp"
#include "sensors/Device.hpp"

#ifdef __unix__

#include <filesystem>
#include <fstream>
#include <LLOG/llog.hpp>
#include <string>

namespace
{
    const std::filesystem::path tempPath = "/sys/class/thermal";

    std::vector<sensors::Device> getCPU()
    {
        const std::filesystem::path cpuinfo = "/proc/cpuinfo";
        std::ifstream file(cpuinfo);

        if(!file)
        {
            llog::Print(llog::pt::error, "Cannot open ", cpuinfo);
        }

        std::vector<sensors::Device> cpus;

        std::string line;
        int cpuCount = 0;
        while(std::getline(file, line))
        {
            if(const std::string name = "model name	: "; line.starts_with(name))
            {
                cpuCount++;
                cpus.push_back({line.substr(std::size(name)), sensors::Device::Type::CPU, -1, -1});
                break; //temporary until more cpu core / more cpu option added
            }
        }

        return cpus;
    }
}

namespace sensors
{
    std::vector<Device> getDevices(Device::Type type)
    {
        auto devices = getCPU();
        return devices;
    }
}
#endif
