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

    std::vector<sensors::Device> CPUinfo()
    {
        const std::filesystem::path cpuinfo = "/proc/cpuinfo";
        std::ifstream file(cpuinfo);

        if(!file)
        {
            llog::Print(llog::pt::error, "Cannot open ", cpuinfo);
            return {};
        }

        std::vector<sensors::Device> cpus;

        std::string line;
        int cpuCount = 0;
        while(std::getline(file, line))
        {
            if(const std::string nameStart = "model name	: "; line.starts_with(nameStart))
            {
                cpuCount++;
                cpus.push_back({line.substr(std::size(nameStart)), sensors::Device::Type::CPU, 0, 0});
                break; //temporary until "more cpu cores" / "more cpus" option added
            }
        }

        return cpus;
    }

    std::vector<sensors::Device> GPUinfo()
    {
        std::vector<sensors::Device> gpus;

        if(std::filesystem::path nvidiaDir = "/proc/driver/nvidia/gpus/"; std::filesystem::is_directory(nvidiaDir))
        {
            for(const auto& gpuPath : std::filesystem::directory_iterator{nvidiaDir}) 
            {
                std::ifstream file(gpuPath.path()/"information");
                
                std::string line;
                while(std::getline(file, line))
                {
                    if(std::string nameStart = "Model:"; line.starts_with(nameStart))
                    {
                        gpus.push_back({line.substr(std::size(nameStart)), sensors::Device::Type::GPU, 0, 0});

                    }
                }
            }
        }
        return gpus;

    }
    
    sensors::Device RAMinfo()
    {

        const std::filesystem::path raminfo = "/proc/meminfo";
        std::ifstream file(raminfo);

        sensors::Device ram {
            .name = {},
            .type = sensors::Device::Type::RAM,
            .temperature = 0,
            .load = 0
        };

        if(!file)
        {
            llog::Print(llog::pt::error, "Cannot open ", raminfo);
            return {};
        }

        std::string line;
        while(std::getline(file, line))
        {
            if(const std::string nameStart = "MemTotal:"; line.starts_with(nameStart))
            {
                auto totalram = line.substr(std::size(nameStart));
                std::erase(totalram, ' ');
                totalram.pop_back();
                totalram.pop_back();
                
                return sensors::Device{std::to_string(static_cast<int>(std::stoi(totalram)*0.000001)) + " GB", sensors::Device::Type::RAM, 0, 0};
            }
        }

        return {};
    }
}

namespace sensors
{
    std::vector<Device> getDevices(Device::Type type)
    {
        std::vector<Device> devices;
        if(type == Device::Type::CPU || type == Device::Type::Any)
        {
            auto cpuInfo = CPUinfo();
            devices.insert(std::end(devices), std::begin(cpuInfo), std::end(cpuInfo));
        }

        if(type == Device::Type::GPU || type == Device::Type::Any)
        {
            auto gpuInfo = GPUinfo();
            devices.insert(std::end(devices), std::begin(gpuInfo), std::end(gpuInfo));
        }

        if(type == Device::Type::RAM || type == Device::Type::Any)
        {
            auto ramInfo = RAMinfo();
            devices.push_back(ramInfo);
        }
        return devices;
    }
}
#endif
