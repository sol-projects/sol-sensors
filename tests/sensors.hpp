#pragma once
#include "sensors/error.hpp"
#include "sensors/sensors.hpp"
#include <DOCTEST/doctest.h>
#include <LLOG/llog.hpp>
#include <cassert>
#include <chrono>
#include <thread>

TEST_CASE("Printing current temperature/usage information")
{
    using namespace std::chrono_literals;

    std::vector<sensors::Device> devices;

    SUBCASE("Gathering info from all devices.")
    {
        devices = sensors::getDevices(sensors::Device::Type::Any);
        CHECK(!devices.empty());
    }

    // SUBCASE("Checking temperature and load.")
    {
        for (auto& device : devices)
        {
            switch (device.type)
            {
                case sensors::Device::Type::CPU:
                    {
                        CHECK(!device.name.empty());
                        llog::Print("Testing CPU:", device.name);

                        auto t1 = std::chrono::high_resolution_clock::now();
                        auto temp = sensors::getTemp(device);
                        auto t2 = std::chrono::high_resolution_clock::now();
                        auto time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
                        if (temp == sensors::error::code)
                        {
                            llog::Print(llog::pt::warning, "CPU temperature not supported.");
                        }
                        else
                        {
                            llog::Print("Temperature:", temp, "Time took:", time.count(), "μs");
                            auto tempPrecision2 = sensors::getTemp(device, 3);
                            llog::Print("Temperature with precision 3:", tempPrecision2);
                        }

                        std::this_thread::sleep_for(150ms);
                        auto load = sensors::getLoad(device);
                        if (load == sensors::error::code)
                        {
                            llog::Print(llog::pt::warning, "CPU load not supported.");
                        }
                        else
                        {
                            for (int i = 0; i < 2; i++)
                            {
                                std::this_thread::sleep_for(150ms);
                                t1 = std::chrono::high_resolution_clock::now();
                                load = sensors::getLoad(device);
                                t2 = std::chrono::high_resolution_clock::now();
                                time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
                                auto loadf = std::to_string(load);
                                llog::Print("Load (precision 0): ", loadf + "%", "Time took:", time.count(), "μs");

                                std::this_thread::sleep_for(150ms);

                                t1 = std::chrono::high_resolution_clock::now();
                                load = sensors::getLoad(device, 2);
                                t2 = std::chrono::high_resolution_clock::now();
                                time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
                                loadf = std::to_string(load);
                                llog::Print("Load (precision 2): ", std::string(loadf.substr(0, loadf.size() - 2) + "." + loadf.substr(loadf.size() - 2, 2) + "%"), "Time took:", time.count(), "μs");
                            }
                        }
                    }
                    break;

                case sensors::Device::Type::CPUThread:
                    {
                        CHECK(!device.name.empty());
                        llog::Print("Testing CPU thread:", device.name);

                        std::this_thread::sleep_for(100ms);
                        auto t1 = std::chrono::high_resolution_clock::now();
                        auto load = sensors::getLoad(device, 1);
                        auto t2 = std::chrono::high_resolution_clock::now();
                        auto time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
                        if (load == sensors::error::code)
                        {
                            llog::Print(llog::pt::warning, "CPU thread load not supported.");
                        }
                        else
                        {
                            auto loadf = std::to_string(load);
                            llog::Print("Load (precision 1): ", std::string(loadf.substr(0, loadf.size() - 1) + "." + loadf.back() + "%"), "Time took:", time.count(), "μs");
                        }
                    }
                    break;

                case sensors::Device::Type::RAM:
                    {
                        CHECK(!device.name.empty());
                        llog::Print("Testing RAM:", device.name);

                        auto t1 = std::chrono::high_resolution_clock::now();
                        auto temp = sensors::getTemp(device);
                        auto t2 = std::chrono::high_resolution_clock::now();
                        auto time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
                        if (temp == sensors::error::code)
                        {
                            llog::Print(llog::pt::warning, "RAM temperature not supported.");
                        }
                        else
                        {
                            llog::Print("Temperature:", temp, "Time took:", time.count(), "μs");
                        }

                        t1 = std::chrono::high_resolution_clock::now();
                        auto load = sensors::getLoad(device);
                        t2 = std::chrono::high_resolution_clock::now();
                        time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
                        if (load == sensors::error::code)
                        {
                            llog::Print(llog::pt::warning, "RAM load not supported.");
                        }
                        else
                        {
                            for (int i = 0; i < 2; i++)
                            {
                                std::this_thread::sleep_for(150ms);
                                t1 = std::chrono::high_resolution_clock::now();
                                load = sensors::getLoad(device);
                                t2 = std::chrono::high_resolution_clock::now();
                                time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
                                auto loadf = std::to_string(load);
                                llog::Print("Load (precision 0): ", loadf + " GB", "Time took:", time.count(), "μs");

                                std::this_thread::sleep_for(150ms);

                                t1 = std::chrono::high_resolution_clock::now();
                                load = sensors::getLoad(device, 2);
                                t2 = std::chrono::high_resolution_clock::now();
                                time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
                                loadf = std::to_string(load);
                                llog::Print("Load (precision 2): ", std::string(loadf.substr(0, loadf.size() - 2) + "." + loadf.substr(loadf.size() - 2, 2) + " GB"), "Time took:", time.count(), "μs");
                            }
                        }
                    }
                    break;

                case sensors::Device::Type::GPU:
                    {
                        CHECK(!device.name.empty());
                        llog::Print("Testing GPU:", device.name);

                        auto t1 = std::chrono::high_resolution_clock::now();
                        auto temp = sensors::getTemp(device);
                        auto t2 = std::chrono::high_resolution_clock::now();
                        auto time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
                        if (temp == sensors::error::code)
                        {
                            llog::Print(llog::pt::warning, "GPU temperature not supported.");
                        }
                        else
                        {
                            llog::Print("Temperature:", temp, "Time took:", time.count(), "μs");
                        }

                        t1 = std::chrono::high_resolution_clock::now();
                        auto load = sensors::getLoad(device);
                        t2 = std::chrono::high_resolution_clock::now();
                        time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
                        if (load == sensors::error::code)
                        {
                            llog::Print(llog::pt::warning, "GPU load not supported.");
                        }
                        else
                        {
                            auto loadf = std::to_string(sensors::getLoad(device));
                            llog::Print("Load:", std::to_string(sensors::getLoad(device)) + "%", "Time took:", time.count(), "μs");
                        }
                    }
                    break;

                case sensors::Device::Type::VRAM:
                    {
                        CHECK(!device.name.empty());
                        llog::Print("Testing VRAM:", device.name);

                        auto t1 = std::chrono::high_resolution_clock::now();
                        auto temp = sensors::getTemp(device);
                        auto t2 = std::chrono::high_resolution_clock::now();
                        auto time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);

                        if (temp == sensors::error::code)
                        {
                            llog::Print(llog::pt::warning, "VRAM temperature not supported.");
                        }
                        else
                        {
                            llog::Print("Temperature:", temp, "Time took:", time.count(), "μs");
                        }

                        t1 = std::chrono::high_resolution_clock::now();
                        auto load = sensors::getLoad(device, 2);
                        t2 = std::chrono::high_resolution_clock::now();
                        time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
                        if (load == sensors::error::code)
                        {
                            llog::Print(llog::pt::warning, "VRAM load not supported.");
                        }
                        else
                        {
                            auto loadf = std::to_string(load);
                            llog::Print("Load (precision 2): ", std::string(loadf.substr(0, loadf.size() - 2) + "." + loadf.substr(loadf.size() - 2, 2) + " GB"), "Time took:", time.count(), "μs");
                        }
                    }
                    break;
                case sensors::Device::Type::Any:
                    {
                        REQUIRE(0);
                        llog::Print(llog::pt::error, "Unknown device type", device.name);
                    }
            }
        }
    }
}
