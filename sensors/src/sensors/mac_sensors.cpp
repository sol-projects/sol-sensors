#include "sensors/sensors.hpp"
#include "sensors/error.hpp"

#ifdef __APPLE__
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mach/mach.h>
#include <sys/sysctl.h>
#include <IOKit/IOKitLib.h>

namespace
{
    int getCPULoad()
    {
        host_cpu_load_info_data_t cpuInfo;
        mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
        if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuInfo, &count) != KERN_SUCCESS)
        {
            return -1;
        }

        uint64_t totalTicks = 0;
        for (int i = 0; i < CPU_STATE_MAX; i++)
        {
            totalTicks += cpuInfo.cpu_ticks[i];
        }

        int load = 100 * (cpuInfo.cpu_ticks[CPU_STATE_USER] + cpuInfo.cpu_ticks[CPU_STATE_SYSTEM]) / totalTicks;
        return load;
    }

    int getCPUTemperature()
    {
        return 0;
    }

    int getRAMSize()
    {
        int64_t ramSize = 0;
        size_t size = sizeof(ramSize);
        sysctlbyname("hw.memsize", &ramSize, &size, nullptr, 0);
        return static_cast<int>(ramSize / (1024 * 1024 * 1024)); // GB
    }

    int getCPUThreadCount()
    {
        int cpuThreads = 0;
        size_t size = sizeof(cpuThreads);
        sysctlbyname("hw.logicalcpu", &cpuThreads, &size, nullptr, 0);
        return cpuThreads;
    }
}

namespace sensors
{
    std::vector<Device> getDevices(Device::Type type)
    {
        std::vector<Device> devices;

        if (type == Device::Type::CPU || type == Device::Type::Any)
        {
            Device cpuDevice("CPU", Device::Type::CPU);
            cpuDevice.load = getCPULoad();
            cpuDevice.temperature = getCPUTemperature();
            devices.push_back(cpuDevice);
        }

        if (type == Device::Type::CPUThread || type == Device::Type::Any)
        {
            Device cpuThreadDevice("CPU Thread", Device::Type::CPUThread);
            cpuThreadDevice.size = getCPUThreadCount(); // Broj CPU niti
            devices.push_back(cpuThreadDevice);
        }

        if (type == Device::Type::RAM || type == Device::Type::Any)
        {
            Device ramDevice("RAM", Device::Type::RAM);
            ramDevice.size = getRAMSize(); // Veličina RAM-a
            devices.push_back(ramDevice);
        }

        return devices;
    }

    int getLoad(const Device& device, int precision)
    {
        return error::code;
    }

    int getTemp(const Device& device, int precision)
    {
        return error::code;
    }
}
#endif
