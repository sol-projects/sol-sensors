#include "sensors/Device.hpp"
#include "sensors/sensors.hpp"

#ifdef __linux__

#include "sensors/error.hpp"
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

    [[maybe_unused]] int findNumCpuCores()
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
                break;
            }
        }

        return cpus;
    }

    std::vector<sensors::Device> CPUThreadinfo()
    {
        std::string parentName = CPUinfo().at(0).name;
        std::ifstream file(cpuUsageInfo);

        if (!file)
        {
            llog::Print(llog::pt::error, "Cannot open ", cpuUsageInfo);
            return {};
        }

        std::vector<sensors::Device> threads;

        std::string line;
        std::getline(file, line);
        int threadNum = 0;
        while (std::getline(file, line))
        {
            if (line.starts_with("cpu"))
            {
                threads.push_back({ sensors::Device::generateID(), parentName + " thread #" + std::to_string(threadNum), sensors::Device::Type::CPUThread, 0, 0 });
                threadNum++;
            }
            else
            {
                break;
            }
        }

        return threads;
    }

    int CPUload(int thread = -1)
    {
        std::ifstream file(cpuUsageInfo);

        if (!file)
        {
            llog::Print(llog::pt::error, "Cannot open file", cpuUsageInfo);
            return sensors::error::code;
        }

        static std::vector<std::size_t> prevIdle(1, 0);
        static std::vector<std::size_t> idle(1, 0);
        static std::vector<std::size_t> prevUser(1, 0);
        static std::vector<std::size_t> user(1, 0);
        static std::vector<std::size_t> prevNice(1, 0);
        static std::vector<std::size_t> nice(1, 0);
        static std::vector<std::size_t> prevSystem(1, 0);
        static std::vector<std::size_t> system(1, 0);
        static std::vector<std::size_t> prevIoWait(1, 0);
        static std::vector<std::size_t> ioWait(1, 0);
        static std::vector<std::size_t> prevIrq(1, 0);
        static std::vector<std::size_t> irq(1, 0);
        static std::vector<std::size_t> prevSoftIrq(1, 0);
        static std::vector<std::size_t> softIrq(1, 0);
        static std::vector<std::size_t> prevSteal(1, 0);
        static std::vector<std::size_t> steal(1, 0);

        // -1 is average of all threads, it gets initialized first in the vector
        if (thread != -1)
        {
            std::string line;
            std::getline(file, line);
            while (std::getline(file, line))
            {
                if (line.at(3) - '0' == thread)
                {
                    break;
                }
            }
        }

        thread += 1;
        if (static_cast<int>(std::size(prevIdle)) < thread)
        {
            llog::Print(llog::pt::warning, "Threads initialized in non-sequential order, could be incorrect.", llog::Location());
        }

        if (static_cast<int>(std::size(prevIdle)) <= thread && thread != 0)
        {
            prevIdle.push_back(0);
            idle.push_back(0);
            prevUser.push_back(0);
            user.push_back(0);
            prevNice.push_back(0);
            nice.push_back(0);
            prevSystem.push_back(0);
            system.push_back(0);
            prevIoWait.push_back(0);
            ioWait.push_back(0);
            prevIrq.push_back(0);
            irq.push_back(0);
            prevSoftIrq.push_back(0);
            softIrq.push_back(0);
            prevSteal.push_back(0);
            steal.push_back(0);
        }

        prevIdle.at(thread) = idle.at(thread);
        prevUser.at(thread) = user.at(thread);
        prevNice.at(thread) = nice.at(thread);
        prevSystem.at(thread) = system.at(thread);
        prevIoWait.at(thread) = ioWait.at(thread);
        prevIrq.at(thread) = irq.at(thread);
        prevSoftIrq.at(thread) = softIrq.at(thread);
        prevSteal.at(thread) = steal.at(thread);
        std::string cpuName;
        file >> cpuName;

        file >> user.at(thread);
        file >> nice.at(thread);
        file >> system.at(thread);
        file >> idle.at(thread);
        file >> ioWait.at(thread);
        file >> irq.at(thread);
        file >> softIrq.at(thread);
        file >> steal.at(thread);

        if (prevIdle.at(thread) == 0)
        {
            prevIdle.at(thread) = idle.at(thread);
            prevUser.at(thread) = user.at(thread);
            prevNice.at(thread) = nice.at(thread);
            prevSystem.at(thread) = system.at(thread);
            prevIoWait.at(thread) = ioWait.at(thread);
            prevIrq.at(thread) = irq.at(thread);
            prevSoftIrq.at(thread) = softIrq.at(thread);
            prevSteal.at(thread) = steal.at(thread);
            return 0;
        }

        auto fullPrevIdle = prevIdle.at(thread) + prevIoWait.at(thread);
        auto fullIdle = idle.at(thread) + ioWait.at(thread);

        auto prevNonIdle = prevUser.at(thread) + prevNice.at(thread) + prevSystem.at(thread) + prevIrq.at(thread) + prevSoftIrq.at(thread) + prevSteal.at(thread);
        auto nonIdle = user.at(thread) + nice.at(thread) + system.at(thread) + irq.at(thread) + softIrq.at(thread) + steal.at(thread);
        auto prevTotal = fullPrevIdle + prevNonIdle;
        auto total = fullIdle + nonIdle;

        auto totaldiff = total - prevTotal;
        auto idlediff = fullIdle - fullPrevIdle;
        return static_cast<int>((1000 * static_cast<float>(totaldiff - idlediff) / totaldiff + 1));
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

        if (type == Device::Type::CPUThread || type == Device::Type::Any)
        {
            auto cpuThreadInfo = CPUThreadinfo();
            devices.insert(std::end(devices), std::begin(cpuThreadInfo), std::end(cpuThreadInfo));

            if (!cpuThreadInfo.empty())
            {
                for (const auto& thread : cpuThreadInfo)
                    getLoad(thread);
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
                    return CPUload();
                }
            case Device::Type::CPUThread:
                {
                    auto iter = std::end(device.name) - 1;
                    std::string threadNumber;
                    while (*iter != '#')
                    {
                        threadNumber.insert(0, 1, *iter);
                        --iter;
                    }

                    return CPUload(std::stoi(threadNumber));
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

                        return error::code;
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

                    return error::code;
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
                    const std::string cpuTempPath = findCpuTempPath();
                    if (cpuTempPath.empty())
                    {
                        return error::code;
                    }

                    std::ifstream tempFile(cpuTempPath);

                    if (!tempFile)
                    {
                        llog::Print(llog::pt::error, "Cannot load CPU temperature from file:", cpuTempPath);
                        return error::code;
                    }

                    tempFile.seekg(0);
                    int temp = 0;
                    tempFile >> temp;
                    return temp / 1000;
                }
            case Device::Type::CPUThread:
                {
                    return error::code;
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
                    return error::code;
                }
            case Device::Type::VRAM:
                {
                    if (gpuType == GPUType::Nvidia)
                    {
                        if (auto memTemp = nvidiasmiQuery("temperature.memory"); memTemp != "N/A")
                        {
                            return std::stoi(memTemp);
                        }

                        return error::code;
                    }

                    return error::code;
                }
            case Device::Type::Any:
                llog::Print(llog::pt::error, "Getting temperature from invalid device type with name:", device.name);
        }

        return error::code;
    }
}
#endif
