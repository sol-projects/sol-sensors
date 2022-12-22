#include "sensors/Device.hpp"
#include "sensors/sensors.hpp"

#ifdef _WIN32

#include "sensors/error.hpp"
#include <Intrin.h>
#include <LLOG/llog.hpp>
#include <Windows.h>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <processthreadsapi.h>
#include <string>
#include <sysinfoapi.h>
#include <tchar.h>
#include <processthreadsapi.h>
#include <sysinfoapi.h>
#include <Intrin.h>
#include <atomic>
#include <iostream>
#include <tchar.h> 
#define NVAPI_MAX_PHYSICAL_GPUS 64
#define NVAPI_MAX_USAGES_PER_GPU 34

// function pointer types
typedef int* (*NvAPI_QueryInterface_t)(unsigned int offset);
typedef int (*NvAPI_Initialize_t)();
typedef int (*NvAPI_EnumPhysicalGPUs_t)(int** handles, int* count);
typedef int (*NvAPI_GPU_GetUsages_t)(int* handle, unsigned int* usages);
namespace
{

    // declare variables you can use in this file
    enum class GPUType
    {
        Nvidia,
        Amd,
        Intel,
        Unknown
    };

    GPUType GetGPUType()
    {
        int i = 0;
        std::string out;
        std::string fullQuery = "wmic path win32_VideoController get AdapterCompatibility";
        FILE* pFile = _popen(fullQuery.c_str(), "r");
        char buffer[256];
        while (!feof(pFile))
        {
            if (fgets(buffer, 128, pFile) != NULL)
            {
                if (i > 21)
                {
                    out += buffer;
                }
            }
            i++;
        }
        _pclose(pFile);
        if (out == "NVIDIA")
        {
            return GPUType::Nvidia;
        }
        else if (out == "AMD")
        {
            return GPUType::Amd;
        }
        return GPUType::Unknown;
    }
    const auto gpuType = GetGPUType();
    static float CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks)
    {
        static unsigned long long _previousTotalTicks = 0;
        static unsigned long long _previousIdleTicks = 0;

        unsigned long long totalTicksSinceLastTime = totalTicks - _previousTotalTicks;
        unsigned long long idleTicksSinceLastTime = idleTicks - _previousIdleTicks;

        float ret = 1.0f - ((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime) / totalTicksSinceLastTime : 0);

        _previousTotalTicks = totalTicks;
        _previousIdleTicks = idleTicks;
        return ret;
    }
     static unsigned long long FileTimeToInt64(const FILETIME & ft) {return (((unsigned long long)(ft.dwHighDateTime))<<32)|((unsigned long long)ft.dwLowDateTime);}

    std::vector<sensors::Device> CPUinfo()
    {
        std::string out;
        std::string fullQuery = "wmic cpu get name";
        FILE* pFile = _popen(fullQuery.c_str(), "r");
        char buffer[256];
        while (!feof(pFile))
        {
            if (fgets(buffer, 128, pFile) != NULL)
                out += buffer;
        }
        out.erase(std::remove(out.begin(), out.end(), '\n'), out.cend());
        _pclose(pFile);
        std::vector<sensors::Device> cpus;
        cpus.push_back({ sensors::Device::generateID(), out, sensors::Device::Type::CPU, 0, 0 });
        return cpus;
    }
    std::vector<sensors::Device> GPUinfo()
    {
        std::string out;
        std::string fullQuery = "wmic path win32_VideoController get name";
        FILE* pFile = _popen(fullQuery.c_str(), "r");
        char buffer[256];
        while (!feof(pFile))
        {
            if (fgets(buffer, 128, pFile) != NULL)
                out += buffer;
        }
        _pclose(pFile);
        out.erase(std::remove(out.begin(), out.end(), '\n'), out.cend());
        std::vector<sensors::Device> gpus;
        gpus.push_back({ sensors::Device::generateID(), out, sensors::Device::Type::GPU, 0, 0 });
        return gpus;
    }
    sensors::Device RAMinfo()
    {
        sensors::Device ram {
            ram.id = sensors::Device::generateID(),
            ram.name = {},
            ram.type = sensors::Device::Type::RAM,
            ram.temperature = 0,
            ram.load = 0
        };
        MEMORYSTATUSEX memstat = { sizeof(memstat) };
        if (!GlobalMemoryStatusEx(&memstat))
            std::cerr << "Ram error";
        int a = memstat.ullTotalPhys / 1000000;
        return sensors::Device { sensors::Device::generateID(), std::to_string(static_cast<int>(a *0.001 )) + " GB RAM", sensors::Device::Type::RAM, 0, 0 };
    }

    std::vector<sensors::Device> VRAMinfo()
    {
        std::string out;
        std::string test;
        std::string stevilo;
        std::string fullQuery = "wmic path win32_VideoController get AdapterRam";
        FILE* pFile = _popen(fullQuery.c_str(), "r");
        char buffer[128];
        while (!feof(pFile))
        {
            if (fgets(buffer, 128, pFile) != NULL)
            {
                if (isdigit(buffer[0]))
                {
                    out += buffer;
                }
            }
        }
        _pclose(pFile);
        unsigned long int rez = stoul(out);
        std::vector<sensors::Device> vrams;
        if (gpuType == GPUType::Nvidia)
        {
            vrams.push_back({ sensors::Device::generateID(), (rez / 1024 / 1024) + " VRAM", sensors::Device::Type::VRAM, 0, 0 });
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
                    FILETIME idleTime, kernelTime, userTime;
                    int a = 100 * (GetSystemTimes(&idleTime, &kernelTime, &userTime) ? CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime) + FileTimeToInt64(userTime)) : -1.0f);
                    return a;
                }
            case Device::Type::GPU:
                {
                    if (gpuType == GPUType::Nvidia)
                    {
                        std::string fullQuery = "nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader";
                        FILE* pFile = _popen(fullQuery.c_str(), "r");

                        char buffer[256];
                        char* out = fgets(buffer, sizeof(buffer), pFile);

                        _pclose(pFile);
                        return std::stoi(std::string(out, out + std::strlen(out) - 1));
                    }

                    return error::code;
                }
            case Device::Type::RAM:
                {
                    MEMORYSTATUSEX memstat = { sizeof(memstat) };
                    if (!GlobalMemoryStatusEx(&memstat))
                        return 0;
                    int a = (memstat.ullTotalPhys - memstat.ullAvailPhys) / 100000000;
                    return a;
                }
            case Device::Type::VRAM:
                {
                    if (gpuType == GPUType::Nvidia)
                    {

                        std::string memUsedStr = "nvidia-smi --query-gpu=memory.used --format=csv,noheader";
                        FILE* pFile = _popen(memUsedStr.c_str(), "r");

                        char buffer[256];
                        char* out = fgets(buffer, sizeof(buffer), pFile);

                        _pclose(pFile);
                        return std::stoi(memUsedStr.substr(0, std::size(memUsedStr) - 4)) * 0.01;
                    }

                    return 1;
                }
            case Device::Type::Any:
                {
                }
        }

        return 0;
    }

    int getTemp(const Device& device)
    {
        switch (device.type)
        {
            case Device::Type::CPU:
                {
                    return 1;
                }
            case Device::Type::GPU:
                {
                    if (gpuType == GPUType::Nvidia)
                    {
                        std::string fullQuery = "nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader";
                        FILE* pFile = _popen(fullQuery.c_str(), "r");
                        std::string out;
                        char buffer[10];
                        if (fgets(buffer, 128, pFile) != NULL){
                            out += buffer;
                        }
                        _pclose(pFile);
                        return std::stoi(out);
                    }

                    break;
                }
            case Device::Type::RAM:
                {
                    return 1;
                }
            case Device::Type::VRAM:
                {
                    if (gpuType == GPUType::Nvidia)
                    {
                        std::string fullQuery = "nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader";
                        FILE* pFile = _popen(fullQuery.c_str(), "r");

                        char buffer[256];
                        char* out = fgets(buffer, sizeof(buffer), pFile);

                        _pclose(pFile);
                        if (out != "N/A")
                        {
                            return std::stoi(out);
                        }

                        return 1;
                    }

                    return 1;
                }
            case Device::Type::Any:
                {
                }
        }

        return 1;
    }
}
#endif
