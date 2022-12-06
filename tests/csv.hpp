#pragma once
#include <DOCTEST/doctest.h>
#include <filesystem>
#include "shared/csv/csv.hpp"
#include "sensors/sensors.hpp"

TEST_CASE("Testing saving and reading from a file.")
{
    using namespace std::chrono_literals;
    auto cpu = sensors::getDevices(sensors::Device::Type::CPU)[0];

    SUBCASE("Saving into file at interval 100 ms.")
    {
        csv::Csv cpuCsv(cpu, 100, csv::MeasurementType::Both);

        for(int i = 0; i < 20; i++)
        {
            std::this_thread::sleep_for(100ms);
            cpu.load = sensors::getLoad(cpu);
            cpu.temperature = sensors::getTemp(cpu);
            cpuCsv.add(cpu);
        }
    }
}
