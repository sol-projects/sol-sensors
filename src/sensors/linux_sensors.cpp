#include "sensors/Device.hpp"
#include "sensors/sensors.hpp"

#ifdef __linux__

#include <LLOG/llog.hpp>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    const std::filesystem::path tempPath = "/sys/class/thermal";
    const std::filesystem::path nvidiaDir = "/proc/driver/nvidia/gpus/";
    enum class GPUType
    {
        Nvidia, Amd, Intel, Unknown
    };

    GPUType findGpuType()
    {
        if (std::filesystem::is_directory(nvidiaDir))
        {
            return GPUType::Nvidia;
        }

        return GPUType::Unknown;
    }

    const auto gpuType = findGpuType();

    std::string findCpuTempPath()
    {
        for (const auto& thermalPath : std::filesystem::directory_iterator { tempPath })
        {
            if (thermalPath.path().generic_string().starts_with("/sys/class/thermal/thermal_zone"))
            {
                std::ifstream file(thermalPath.path() / "type");

                if (file)
                {
                    std::string line;
                    std::getline(file, line);

                    if (line == "acpitz") // motherboard sensor
                    {
                        return thermalPath.path() / "temp";
                    }

                    if (line == "x86_pkg_temp") // cpu sensor
                    {
                        return thermalPath.path() / "temp";
                    }
                }
                else
                {
                    llog::Print(llog::pt::error, "Cannot open file at:", llog::Location());
                }
            }
        }

        llog::Print(llog::pt::error, "Cannot find CPU temperature file path");
        return "";
    }

    std::vector<sensors::Device> CPUinfo()
    {
        const std::filesystem::path cpuinfo = "/proc/cpuinfo";
        std::ifstream file(cpuinfo);

        if (!file)
        {
            llog::Print(llog::pt::error, "Cannot open ", cpuinfo);
            return {};
        }

        std::vector<sensors::Device> cpus;

        std::string line;
        int cpuCount = 0;
        while (std::getline(file, line))
        {
            if (const std::string nameStart = "model name	: "; line.starts_with(nameStart))
            {
                cpuCount++;
                cpus.push_back({ sensors::Device::generateID(), line.substr(std::size(nameStart)), sensors::Device::Type::CPU, 0, 0 });
                break; // temporary until "more cpu cores" / "more cpus" option added
            }
        }

        return cpus;
    }

    std::vector<sensors::Device> GPUinfo()
    {
        std::vector<sensors::Device> gpus;

        if (gpuType == GPUType::Nvidia)
        {
            for (const auto& gpuPath : std::filesystem::directory_iterator { nvidiaDir })
            {
                std::ifstream file(gpuPath.path() / "information");

                std::string line;
                while (std::getline(file, line))
                {
                    if (std::string nameStart = "Model:"; line.starts_with(nameStart))
                    {
                        gpus.push_back({ sensors::Device::generateID(), line.substr(std::size(nameStart)), sensors::Device::Type::GPU, 0, 0 });
                    }
                }
            }
        }
        else
        {
            //amd
        }
        return gpus;
    }

    sensors::Device RAMinfo()
    {

        const std::filesystem::path raminfo = "/proc/meminfo";
        std::ifstream file(raminfo);

        sensors::Device ram {
            .id = sensors::Device::generateID(),
            .name = {},
            .type = sensors::Device::Type::RAM,
            .temperature = 0,
            .load = 0
        };

        if (!file)
        {
            llog::Print(llog::pt::error, "Cannot open ", raminfo);
            return {};
        }

        std::string line;
        while (std::getline(file, line))
        {
            if (const std::string nameStart = "MemTotal:"; line.starts_with(nameStart))
            {
                auto totalram = line.substr(std::size(nameStart));
                std::erase(totalram, ' ');
                totalram.pop_back();
                totalram.pop_back();

                return sensors::Device { sensors::Device::generateID(), std::to_string(static_cast<int>(std::stoi(totalram) * 0.000001)) + " GB", sensors::Device::Type::RAM, 0, 0 };
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
        if (type == Device::Type::CPU || type == Device::Type::Any)
        {
            auto cpuInfo = CPUinfo();
            devices.insert(std::end(devices), std::begin(cpuInfo), std::end(cpuInfo));
        }

        if (type == Device::Type::GPU || type == Device::Type::Any)
        {
            auto gpuInfo = GPUinfo();
            devices.insert(std::end(devices), std::begin(gpuInfo), std::end(gpuInfo));
        }

        if (type == Device::Type::RAM || type == Device::Type::Any)
        {
            auto ramInfo = RAMinfo();
            devices.push_back(ramInfo);
        }

        return devices;
    }

    int getLoad(const Device& device)
    {
        switch (device.type)
        {
            case Device::Type::CPU:
                {
                    break;
                }
            case Device::Type::GPU:
                {
                    break;
                }
            case Device::Type::RAM:
                {
                    break;
                }
            case Device::Type::Any:
                llog::Print(llog::pt::error, "Getting load from invalid device type with name:", device.name);
        }

        return 0;
    }

    int getTemp(const Device& device)
    {
        switch (device.type)
        {
            case Device::Type::CPU:
                {
                    static const std::string cpuTempPath = findCpuTempPath();
                    static std::ifstream tempFile(cpuTempPath);

                    if (!tempFile)
                    {
                        llog::Print(llog::pt::error, "Cannot load CPU temperature from file:", cpuTempPath);
                        return 0;
                    }

                    tempFile.seekg(0);
                    int temp = 0;
                    tempFile >> temp;
                    return temp / 1000;
                }
            case Device::Type::GPU:
                {
                    if(gpuType == GPUType::Nvidia)
                    {
                        FILE* pFile = popen("nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader", "r");

                        char buffer[32];
                        char* line_p = fgets(buffer, sizeof(buffer), pFile);

                        pclose(pFile);
                        return std::atoi(line_p);
                    }
                    break;
                }
            case Device::Type::RAM:
                {
                }
            case Device::Type::Any:
                llog::Print(llog::pt::error, "Getting temperature from invalid device type with name:", device.name);
        }

        return 0;
    }
}
#endif
