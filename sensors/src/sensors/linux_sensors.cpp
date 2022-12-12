#include "sensors/Device.hpp"
#include "sensors/sensors.hpp"

#ifdef __linux__

#include <LLOG/llog.hpp>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    const std::filesystem::path raminfo = "/proc/meminfo";
    const std::filesystem::path cpuUsageInfo = "/proc/stat";
    const std::filesystem::path tempPath = "/sys/class/thermal";
    const std::filesystem::path nvidiaDir = "/proc/driver/nvidia/gpus/";

    std::string nvidiasmiQuery(const std::string& query)
    {
        std::string fullQuery = "nvidia-smi --query-gpu=" + query + " --format=csv,noheader";
        FILE* pFile = popen(fullQuery.c_str(), "r");

        char buffer[256];
        char* out = fgets(buffer, sizeof(buffer), pFile);

        pclose(pFile);
        return std::string(out, out + std::strlen(out) - 1);
    }

    enum class GPUType
    {
        Nvidia,
        Amd,
        Intel,
        Unknown
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

    int findNumCpuCores()
    {
        std::ifstream file("/proc/stat");

        int numCores = 0;
        if (file)
        {
            std::string line;
            while (std::getline(file, line))
            {
                if (line.starts_with("cpu"))
                {
                    while (std::getline(file, line))
                    {
                        if (!line.starts_with("cpu"))
                        {
                            return numCores;
                        }
                        else
                        {
                            numCores++;
                        }
                    }
                }
            }
        }
        else
        {
            llog::Print(llog::pt::error, "Cannot open file at:", llog::Location());
        }

        llog::Print(llog::pt::error, "Cannot find number of CPU cores.");
        return 0;
    }

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
        while (std::getline(file, line))
        {
            if (const std::string nameStart = "model name	: "; line.starts_with(nameStart))
            {
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
                        auto name = line.substr(std::size(nameStart));
                        while (name[0] == ' ' || name[0] == 9 /*TAB*/)
                        {
                            name.erase(0, 1);
                        }

                        gpus.push_back({ sensors::Device::generateID(), name, sensors::Device::Type::GPU, 0, 0 });
                    }
                }
            }
        }
        else
        {
            // amd
        }

        return gpus;
    }

    sensors::Device RAMinfo()
    {

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

                return sensors::Device { sensors::Device::generateID(), std::to_string(static_cast<int>(std::stoi(totalram) * 0.000001)) + " GB RAM", sensors::Device::Type::RAM, 0, 0 };
            }
        }

        return {};
    }

    std::vector<sensors::Device> VRAMinfo()
    {
        std::vector<sensors::Device> vrams;
        if (gpuType == GPUType::Nvidia)
        {
            vrams.push_back({ sensors::Device::generateID(), nvidiasmiQuery("name") + " " + nvidiasmiQuery("memory.total") + " VRAM", sensors::Device::Type::VRAM, 0, 0 });
        }

        return vrams;
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

            if (!cpuInfo.empty())
            {
                getLoad(cpuInfo[0]);
            }
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

        if (type == Device::Type::VRAM || type == Device::Type::Any)
        {
            auto vramInfo = VRAMinfo();
            devices.insert(std::end(devices), std::begin(vramInfo), std::end(vramInfo));
        }

        return devices;
    }

    int getLoad(const Device& device)
    {
        switch (device.type)
        {
            case Device::Type::CPU:
                {
                    static const int numCores = findNumCpuCores();
                    std::ifstream file(cpuUsageInfo);

                    if (!file)
                    {
                        llog::Print(llog::pt::error, "Cannot open file", cpuUsageInfo);
                    }

                    static std::size_t prevIdle = 0;
                    static std::size_t idle = 0;
                    static std::size_t prevUser = 0;
                    static std::size_t user = 0;
                    static std::size_t prevNice = 0;
                    static std::size_t nice = 0;
                    static std::size_t prevSystem = 0;
                    static std::size_t system = 0;
                    static std::size_t prevIoWait = 0;
                    static std::size_t ioWait = 0;
                    static std::size_t prevIrq = 0;
                    static std::size_t irq = 0;
                    static std::size_t prevSoftIrq = 0;
                    static std::size_t softIrq = 0;
                    static std::size_t prevSteal = 0;
                    static std::size_t steal = 0;

                    prevIdle = idle;
                    prevUser = user;
                    prevNice = nice;
                    prevSystem = system;
                    prevIoWait = ioWait;
                    prevIrq = irq;
                    prevSoftIrq = softIrq;
                    prevSteal = steal;

                    std::string cpuName;
                    file >> cpuName;

                    file >> user;
                    file >> nice;
                    file >> system;
                    file >> idle;
                    file >> ioWait;
                    file >> irq;
                    file >> softIrq;
                    file >> steal;

                    if (prevIdle == 0)
                    {
                        prevIdle = idle;
                        prevUser = user;
                        prevNice = nice;
                        prevSystem = system;
                        prevIoWait = ioWait;
                        prevIrq = irq;
                        prevSoftIrq = softIrq;
                        prevSteal = steal;
                        return 0;
                    }

                    auto fullPrevIdle = prevIdle + prevIoWait;
                    auto fullIdle = idle + ioWait;

                    auto prevNonIdle = prevUser + prevNice + prevSystem + prevIrq + prevSoftIrq + prevSteal;
                    auto nonIdle = user + nice + system + irq + softIrq + steal;
                    auto prevTotal = fullPrevIdle + prevNonIdle;
                    auto total = fullIdle + nonIdle;

                    auto totaldiff = total - prevTotal;
                    auto idlediff = fullIdle - fullPrevIdle;
                    return static_cast<int>((1000 * static_cast<float>(totaldiff - idlediff) / totaldiff + 1));
                }
            case Device::Type::GPU:
                {
                    if (gpuType == GPUType::Nvidia)
                    {
                        return std::stoi(nvidiasmiQuery("utilization.gpu"));
                    }

                    return 0;
                }
            case Device::Type::RAM:
                {
                    std::ifstream ramFile(raminfo);
                    auto getRam = [&](const std::string& name) {
                        std::string line;
                        while (std::getline(ramFile, line))
                        {
                            if (line.starts_with(name))
                            {
                                auto ramStr = line.substr(std::size(name));
                                std::erase(ramStr, ' ');
                                ramStr.pop_back();
                                ramStr.pop_back();
                                return std::stoi(ramStr);
                            }
                        }

                        return 0;
                    };

                    if (!ramFile)
                    {
                        llog::Print(llog::pt::error, "Cannot load Memory usage from file:", raminfo);
                    }

                    static int totalRam = getRam("MemTotal:");
                    int freeRam = getRam("MemAvailable:");

                    return (totalRam - freeRam) * 0.00001;
                }
            case Device::Type::VRAM:
                {
                    if (gpuType == GPUType::Nvidia)
                    {
                        auto memUsedStr = nvidiasmiQuery("memory.used");
                        return std::stoi(memUsedStr.substr(0, std::size(memUsedStr) - 4)) * 0.01;
                    }

                    return 0;
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
                    std::ifstream tempFile(cpuTempPath);

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
                    if (gpuType == GPUType::Nvidia)
                    {
                        return std::stoi(nvidiasmiQuery("temperature.gpu"));
                    }

                    break;
                }
            case Device::Type::RAM:
                {
                    return 0;
                }
            case Device::Type::VRAM:
                {
                    if (gpuType == GPUType::Nvidia)
                    {
                        if (auto memTemp = nvidiasmiQuery("temperature.memory"); memTemp != "N/A")
                        {
                            return std::stoi(memTemp);
                        }

                        return 0;
                    }

                    return 0;
                }
            case Device::Type::Any:
                llog::Print(llog::pt::error, "Getting temperature from invalid device type with name:", device.name);
        }

        return 0;
    }
}
#endif
