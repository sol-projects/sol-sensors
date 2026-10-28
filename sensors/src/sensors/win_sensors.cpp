#include "sensors/error.hpp"
#include "sensors/sensors.hpp"

#ifdef _WIN32
#include <dxgi1_2.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <windows.h>
#if __has_include(<d3dkmthk.h>)
#include <d3dkmthk.h>
#define SENSORS_WDDM_TEMPERATURE
#endif
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>

namespace
{
    int scaledValue(double value, int precision)
    {
        value *= std::pow(10, precision);
        if (precision < 0 || precision > 6 || !std::isfinite(value) || value < 0 || value > std::numeric_limits<int>::max())
        {
            return sensors::error::code;
        }
        return static_cast<int>(value);
    }

    struct GPU
    {
        std::string name;
        LUID luid;
        std::wstring counterId;
    };

    struct Counters
    {
        PDH_HQUERY query = nullptr;
        std::map<std::wstring, PDH_HCOUNTER> handles;
        std::vector<GPU> gpus;
        std::vector<std::wstring> threads;
        std::chrono::steady_clock::time_point lastSample {};
        PDH_STATUS status = PDH_NO_DATA;

        PDH_HCOUNTER add(const std::wstring& path)
        {
            PDH_HCOUNTER counter = nullptr;
            if (query && PdhAddEnglishCounterW(query, path.c_str(), 0, &counter) == ERROR_SUCCESS)
            {
                handles[path] = counter;
            }
            return counter;
        }

        Counters()
        {
            PdhOpenQueryW(nullptr, 0, &query);
            add(L"\\Processor Information(_Total)\\% Processor Time");
            for (WORD group = 0; group < GetActiveProcessorGroupCount(); group++)
            {
                for (DWORD thread = 0; thread < GetActiveProcessorCount(group); thread++)
                {
                    auto path = L"\\Processor Information(" + std::to_wstring(group) + L"," + std::to_wstring(thread) + L")\\% Processor Time";
                    threads.push_back(path);
                    add(path);
                }
            }
            IDXGIFactory1* factory = nullptr;
            if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory))))
            {
                IDXGIAdapter1* adapter = nullptr;
                for (UINT index = 0; factory->EnumAdapters1(index, &adapter) == S_OK; index++)
                {
                    DXGI_ADAPTER_DESC1 desc {};
                    if (SUCCEEDED(adapter->GetDesc1(&desc)) && !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
                    {
                        int size = WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, nullptr, 0, nullptr, nullptr);
                        std::string name(size > 0 ? size : 1, '\0');
                        if (size > 0)
                        {
                            WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, name.data(), size, nullptr, nullptr);
                            name.pop_back();
                        }
                        name += " (GPU " + std::to_string(gpus.size()) + ")";
                        wchar_t id[64];
                        std::swprintf(id, 64, L"luid_0x%08x_0x%08x", static_cast<unsigned>(desc.AdapterLuid.HighPart), static_cast<unsigned>(desc.AdapterLuid.LowPart));
                        gpus.push_back({ name, desc.AdapterLuid, id });
                        add(L"\\GPU Engine(*" + std::wstring(id) + L"*)\\Utilization Percentage");
                        add(L"\\GPU Adapter Memory(" + std::wstring(id) + L"*)\\Dedicated Usage");
                    }
                    adapter->Release();
                }
                factory->Release();
            }
            sample();
        }

        ~Counters()
        {
            if (query)
            {
                PdhCloseQuery(query);
            }
        }

        void sample()
        {
            auto now = std::chrono::steady_clock::now();
            if (now - lastSample >= std::chrono::milliseconds(100))
            {
                status = query ? PdhCollectQueryData(query) : PDH_NO_DATA;
                lastSample = now;
            }
        }

        double value(const std::wstring& path, bool array = false, bool engines = false)
        {
            sample();
            auto found = handles.find(path);
            if (found == handles.end() || status != ERROR_SUCCESS)
            {
                return -1;
            }
            if (!array)
            {
                PDH_FMT_COUNTERVALUE value {};
                if (PdhGetFormattedCounterValue(found->second, PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS
                    && (value.CStatus == PDH_CSTATUS_VALID_DATA || value.CStatus == PDH_CSTATUS_NEW_DATA))
                {
                    return value.doubleValue;
                }
                return -1;
            }
            DWORD bytes = 0, count = 0;
            if (PdhGetFormattedCounterArrayW(found->second, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &bytes, &count, nullptr) != static_cast<PDH_STATUS>(PDH_MORE_DATA))
            {
                return -1;
            }
            std::vector<unsigned char> buffer(bytes);
            auto items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
            if (PdhGetFormattedCounterArrayW(found->second, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &bytes, &count, items) != ERROR_SUCCESS)
            {
                return -1;
            }
            std::map<std::wstring, double> totals;
            bool valid = false;
            for (DWORD i = 0; i < count; i++)
            {
                if (items[i].FmtValue.CStatus != PDH_CSTATUS_VALID_DATA && items[i].FmtValue.CStatus != PDH_CSTATUS_NEW_DATA)
                {
                    continue;
                }
                std::wstring name = items[i].szName;
                auto physical = name.find(L"_phys_");
                auto key = engines && physical != std::wstring::npos ? name.substr(physical) : L"memory";
                totals[key] += items[i].FmtValue.doubleValue;
                valid = true;
            }
            double result = -1;
            for (const auto& [name, total] : totals)
            {
                result = std::max(result, total);
            }
            return valid ? (engines ? std::clamp(result, 0.0, 100.0) : result) : -1;
        }
    };

    Counters& counters()
    {
        static Counters instance;
        return instance;
    }

    std::string CPUname()
    {
        char name[256] {};
        DWORD size = sizeof(name);
        if (RegGetValueA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "ProcessorNameString", RRF_RT_REG_SZ, nullptr, name, &size) == ERROR_SUCCESS)
        {
            return name;
        }
        return "CPU";
    }
}

namespace sensors
{
    std::vector<Device> getDevices(Device::Type type)
    {
        std::vector<Device> devices;
        auto& info = counters();
        if (type == Device::Type::CPU || type == Device::Type::Any)
        {
            devices.push_back({ Device::generateID(), CPUname(), Device::Type::CPU, 0, 0 });
        }
        if (type == Device::Type::CPUThread || type == Device::Type::Any)
        {
            for (std::size_t i = 0; i < info.threads.size(); i++)
            {
                devices.push_back({ Device::generateID(), CPUname() + " thread #" + std::to_string(i), Device::Type::CPUThread, 0, 0 });
            }
        }
        if (type == Device::Type::RAM || type == Device::Type::Any)
        {
            MEMORYSTATUSEX memory {};
            memory.dwLength = sizeof(memory);
            if (GlobalMemoryStatusEx(&memory))
            {
                devices.push_back({ Device::generateID(), std::to_string(memory.ullTotalPhys / 1000000000) + " GB RAM", Device::Type::RAM, 0, 0 });
            }
        }
        for (auto gpuType : { Device::Type::GPU, Device::Type::VRAM })
        {
            if (type == gpuType || type == Device::Type::Any)
            {
                for (const auto& gpu : info.gpus)
                {
                    devices.push_back({ Device::generateID(), gpu.name + (gpuType == Device::Type::VRAM ? " VRAM" : ""), gpuType, 0, 0 });
                }
            }
        }
        return devices;
    }

    int getLoad(const Device& device, int precision)
    {
        auto& info = counters();
        switch (device.type)
        {
            case Device::Type::CPU:
                return scaledValue(info.value(L"\\Processor Information(_Total)\\% Processor Time"), precision);
            case Device::Type::CPUThread:
                {
                    auto marker = device.name.rfind('#');
                    if (marker == std::string::npos)
                    {
                        return error::code;
                    }
                    auto thread = device.name.substr(marker + 1);
                    for (std::size_t i = 0; i < info.threads.size(); i++)
                    {
                        if (thread == std::to_string(i))
                        {
                            return scaledValue(info.value(info.threads[i]), precision);
                        }
                    }
                    return error::code;
                }
            case Device::Type::RAM:
                {
                    MEMORYSTATUSEX memory {};
                    memory.dwLength = sizeof(memory);
                    return GlobalMemoryStatusEx(&memory) ? scaledValue((memory.ullTotalPhys - memory.ullAvailPhys) / 1e9, precision) : error::code;
                }
            case Device::Type::GPU:
            case Device::Type::VRAM:
                for (const auto& gpu : info.gpus)
                {
                    if (device.name == gpu.name && device.type == Device::Type::GPU)
                    {
                        return scaledValue(info.value(L"\\GPU Engine(*" + gpu.counterId + L"*)\\Utilization Percentage", true, true), precision);
                    }
                    if (device.name == gpu.name + " VRAM" && device.type == Device::Type::VRAM)
                    {
                        auto bytes = info.value(L"\\GPU Adapter Memory(" + gpu.counterId + L"*)\\Dedicated Usage", true);
                        return bytes < 0 ? error::code : scaledValue(bytes / 1e9, precision);
                    }
                }
                break;
            case Device::Type::Any:
                break;
        }
        return error::code;
    }

    int getTemp(const Device& device, int precision)
    {
#ifdef SENSORS_WDDM_TEMPERATURE
        if (device.type != Device::Type::GPU)
        {
            return error::code;
        }
        for (const auto& gpu : counters().gpus)
        {
            if (device.name != gpu.name)
            {
                continue;
            }
            D3DKMT_OPENADAPTERFROMLUID adapter {};
            adapter.AdapterLuid = gpu.luid;
            if (D3DKMTOpenAdapterFromLuid(&adapter) < 0)
            {
                return error::code;
            }
            D3DKMT_ADAPTER_PERFDATA data {};
            D3DKMT_QUERYADAPTERINFO query {};
            query.hAdapter = adapter.hAdapter;
            query.Type = KMTQAITYPE_ADAPTERPERFDATA;
            query.pPrivateDriverData = &data;
            query.PrivateDriverDataSize = sizeof(data);
            auto status = D3DKMTQueryAdapterInfo(&query);
            D3DKMT_CLOSEADAPTER close {};
            close.hAdapter = adapter.hAdapter;
            D3DKMTCloseAdapter(&close);
            return status >= 0 && data.Temperature > 0 ? scaledValue(data.Temperature / 10.0, precision) : error::code;
        }
#else
        // shortcut: MinGW lacks WDDM temperature headers; use the Windows SDK to enable GPU temperature.
        (void)device;
        (void)precision;
#endif
        return error::code;
    }
}
#endif
