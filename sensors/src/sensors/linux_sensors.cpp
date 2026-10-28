#ifdef __linux__

#include "sensors/Device.hpp"
#include "sensors/error.hpp"
#include "sensors/sensors.hpp"
#include <LLOG/llog.hpp>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <algorithm>
#include <limits>
#include <sstream>
#include <unordered_map>

namespace
{
    const std::filesystem::path raminfo = "/proc/meminfo";
    const std::filesystem::path cpuUsageInfo = "/proc/stat";
    const std::filesystem::path tempPath = "/sys/class/thermal";
    const std::filesystem::path nvidiaDir = "/proc/driver/nvidia/gpus/";

    std::unordered_map<std::string, std::filesystem::path> amdPaths;
    std::unordered_map<std::string, std::string> nvidiaIds;

    int scaledValue(double value, int precision)
    {
        value *= std::pow(10, precision);
        if (!std::isfinite(value) || value < 0 || value > std::numeric_limits<int>::max())
        {
            return sensors::error::code;
        }
        return static_cast<int>(value);
    }

    int fileValue(const std::filesystem::path& path, double divisor, int precision)
    {
        std::ifstream file(path);
        double value;
        return file >> value ? scaledValue(value / divisor, precision) : sensors::error::code;
    }

    std::string nvidiasmiQuery(const std::string& query, const std::string& id)
    {
        std::string fullQuery = "nvidia-smi -i " + id + " --query-gpu=" + query + " --format=csv,noheader,nounits 2>/dev/null";
        FILE* pFile = popen(fullQuery.c_str(), "r");
        if (!pFile)
        {
            return {};
        }
        char buffer[256];
        std::string result;
        if (fgets(buffer, sizeof(buffer), pFile))
        {
            result = buffer;
            result.erase(result.find_last_not_of("\r\n") + 1);
        }
        return pclose(pFile) == 0 ? result : std::string {};
    }

    int nvidiaValue(const sensors::Device& device, const std::string& query, double divisor, int precision)
    {
        auto found = nvidiaIds.find(device.name);
        if (found == nvidiaIds.end())
        {
            return sensors::error::code;
        }
        std::istringstream value(nvidiasmiQuery(query, found->second));
        double number;
        return value >> number ? scaledValue(number / divisor, precision) : sensors::error::code;
    }

    std::vector<sensors::Device> AMDinfo(const std::filesystem::path& root = "/sys/class/drm")
    {
        std::vector<sensors::Device> devices;
        std::error_code error;
        for (std::filesystem::directory_iterator it(root, error), end; !error && it != end; it.increment(error))
        {
            auto card = it->path().filename().string();
            if (!card.starts_with("card") || card.find_first_not_of("0123456789", 4) != std::string::npos || card.size() == 4)
            {
                continue;
            }
            auto path = it->path() / "device";
            std::ifstream vendorFile(path / "vendor");
            std::string vendor;
            vendorFile >> vendor;
            if (vendor != "0x1002")
            {
                continue;
            }
            std::ifstream nameFile(path / "product_name");
            std::string name;
            std::getline(nameFile, name);
            if (name.empty())
            {
                name = "AMD GPU";
            }
            name += " (" + card + ")";
            amdPaths[name] = path;
            amdPaths[name + " VRAM"] = path;
            devices.push_back({ sensors::Device::generateID(), name, sensors::Device::Type::GPU, 0, 0 });
        }
        return devices;
    }

    int AMDtemp(const std::filesystem::path& path, bool memory, int precision)
    {
        std::error_code error;
        for (std::filesystem::directory_iterator it(path / "hwmon", error), end; !error && it != end; it.increment(error))
        {
            if (!memory)
            {
                return fileValue(it->path() / "temp1_input", 1000, precision);
            }
            for (std::filesystem::directory_iterator sensor(it->path(), error), sensorEnd; !error && sensor != sensorEnd; sensor.increment(error))
            {
                auto name = sensor->path().filename().string();
                if (!name.starts_with("temp") || !name.ends_with("_label"))
                {
                    continue;
                }
                std::ifstream labelFile(sensor->path());
                std::string label;
                std::getline(labelFile, label);
                if (label == "mem" || label == "memory")
                {
                    return fileValue(it->path() / (name.substr(0, name.size() - 6) + "_input"), 1000, precision);
                }
            }
        }
        return sensors::error::code;
    }

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

        std::error_code error;
        for (std::filesystem::directory_iterator it("/sys/class/hwmon", error), end; !error && it != end; it.increment(error))
        {
            std::ifstream file(it->path() / "name");
            std::string name;
            file >> name;
            if (name == "k10temp" || name == "coretemp")
            {
                return it->path() / "temp1_input";
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

    int CPUload(int precision, int thread = -1)
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

        // -1 is the average of all threads, it gets initialized first in the vector
        if (thread != -1)
        {
            std::string line;
            for (int i = 0; i <= thread; i++)
            {
                std::getline(file, line);
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

        int precisionFactor = 100;
        for (int i = 0; i < precision; i++)
        {
            precisionFactor *= 10;
        }

        return totaldiff ? static_cast<int>((precisionFactor * static_cast<double>(totaldiff - idlediff)) / totaldiff) : 0;
    }

    std::vector<sensors::Device> GPUinfo()
    {
        std::vector<sensors::Device> gpus;

        if (std::filesystem::is_directory(nvidiaDir))
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

                        if (std::any_of(gpus.begin(), gpus.end(), [&name](const auto& gpu) { return gpu.name == name; }))
                        {
                            name += " (" + gpuPath.path().filename().string() + ")";
                        }
                        nvidiaIds[name] = gpuPath.path().filename().string();
                        gpus.push_back({ sensors::Device::generateID(), name, sensors::Device::Type::GPU, 0, 0 });
                    }
                }
            }
        }
        auto amdGpus = AMDinfo();
        gpus.insert(gpus.end(), amdGpus.begin(), amdGpus.end());
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
        for (const auto& gpu : GPUinfo())
        {
            auto name = gpu.name + " VRAM";
            if (auto found = nvidiaIds.find(gpu.name); found != nvidiaIds.end())
            {
                nvidiaIds[name] = found->second;
            }
            vrams.push_back({ sensors::Device::generateID(), name, sensors::Device::Type::VRAM, 0, 0 });
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

    int getLoad(const Device& device, int precision)
    {
        if (precision < 0 || precision > 6)
        {
            return error::code;
        }
        if (auto found = amdPaths.find(device.name); found != amdPaths.end())
        {
            return fileValue(found->second / (device.type == Device::Type::VRAM ? "mem_info_vram_used" : "gpu_busy_percent"), device.type == Device::Type::VRAM ? 1e9 : 1, precision);
        }
        switch (device.type)
        {
            case Device::Type::CPU:
                {
                    return CPUload(precision);
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

                    return CPUload(precision, std::stoi(threadNumber));
                }
            case Device::Type::GPU:
                {
                    return nvidiaValue(device, "utilization.gpu", 1, precision);
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

                    if (totalRam == error::code || freeRam == error::code)
                    {
                        return error::code;
                    }
                    auto precisionFactor = 0.000001F;
                    precisionFactor *= std::pow(10, precision);
                    return (totalRam - freeRam) * precisionFactor;
                }
            case Device::Type::VRAM:
                {
                    return nvidiaValue(device, "memory.used", 1e9 / (1024 * 1024), precision);
                }
            case Device::Type::Any:
                llog::Print(llog::pt::error, "Getting load from invalid device type with name:", device.name);
        }

        return 0;
    }

    int getTemp(const Device& device, int precision)
    {
        if (precision < 0 || precision > 6)
        {
            return error::code;
        }
        if (auto found = amdPaths.find(device.name); found != amdPaths.end())
        {
            return AMDtemp(found->second, device.type == Device::Type::VRAM, precision);
        }
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

                    double temp;
                    return tempFile >> temp ? scaledValue(temp / 1000.0, precision) : error::code;
                }
            case Device::Type::CPUThread:
                {
                    return error::code;
                }
            case Device::Type::GPU:
                {
                    return nvidiaValue(device, "temperature.gpu", 1, precision);
                }
            case Device::Type::RAM:
                {
                    return error::code;
                }
            case Device::Type::VRAM:
                {
                    return nvidiaValue(device, "temperature.memory", 1, precision);
                }
            case Device::Type::Any:
                llog::Print(llog::pt::error, "Getting temperature from invalid device type with name:", device.name);
        }

        return error::code;
    }
}
#endif
