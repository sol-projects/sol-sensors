#pragma once
#include "LLOG/llog.hpp"
#include "sensors/sensors.hpp"
#include "shared/csv/csv.hpp"
#include <DOCTEST/doctest.h>
#include <filesystem>
#include <thread>

TEST_CASE("Testing saving and reading from a file.")
{
    using namespace std::chrono_literals;

    SUBCASE("Saving into file at W = 40 ms.")
    {
        auto cpu = sensors::getDevices(sensors::Device::Type::CPU)[0];
        csv::Csv cpuCsv(cpu, 100, csv::MeasurementType::Both);
        cpuCsv.removeFileOnDestruct();

        for (int i = 0; i < 20; i++)
        {
            std::this_thread::sleep_for(40ms);
            cpu.load = sensors::getLoad(cpu);
            cpu.temperature = sensors::getTemp(cpu);
            cpuCsv.add(cpu);
        }

        auto loads = cpuCsv.getLastnData(30, 1);
        auto temps = cpuCsv.getLastnData(10, 2);
        std::cout << std::endl;
        llog::Print("Reading CPU loads and temps from csv file:");
        llog::Print(loads);
        llog::Print(temps);
        std::cout << std::endl;
    }

    SUBCASE("Saving into file into a new directory.")
    {
        auto cpu = sensors::getDevices(sensors::Device::Type::CPU)[0];
        csv::Csv cpuCsv(cpu, 100, csv::MeasurementType::Both, "new");
        cpuCsv.removeFileOnDestruct();

        for (int i = 0; i < 2; i++)
        {
            std::this_thread::sleep_for(40ms);
            cpu.load = sensors::getLoad(cpu);
            cpu.temperature = sensors::getTemp(cpu);
            cpuCsv.add(cpu);
        }
    }
}
