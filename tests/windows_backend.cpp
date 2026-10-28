#include "../sensors/src/sensors/win_sensors.cpp"
#include <DOCTEST/doctest.h>
#include <set>
#include <thread>

TEST_CASE("Windows enumeration and native measurements")
{
    CHECK(scaledValue(45.67, 2) == 4567);
    CHECK(scaledValue(-1, 2) == sensors::error::code);
    CHECK(scaledValue(1e20, 2) == sensors::error::code);
    CHECK(scaledValue(1, -1) == sensors::error::code);
    CHECK(scaledValue(1, 7) == sensors::error::code);
    auto devices = sensors::getDevices(sensors::Device::Type::Any);
    REQUIRE(!devices.empty());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::set<int> ids;
    bool cpu = false, ram = false, thread = false;
    for (const auto& device : devices)
    {
        CHECK(!device.name.empty());
        CHECK(ids.insert(device.id).second);
        auto load = sensors::getLoad(device, 2);
        CHECK((load == sensors::error::code || load >= 0));
        if (device.type == sensors::Device::Type::CPU || device.type == sensors::Device::Type::CPUThread || device.type == sensors::Device::Type::GPU)
        {
            CHECK((load == sensors::error::code || load <= 10000));
        }
        CHECK(sensors::getLoad(device, -1) == sensors::error::code);
        CHECK(sensors::getTemp(device, 7) == sensors::error::code);
        cpu |= device.type == sensors::Device::Type::CPU;
        ram |= device.type == sensors::Device::Type::RAM;
        thread |= device.type == sensors::Device::Type::CPUThread;
    }
    CHECK(cpu);
    CHECK(ram);
    CHECK(thread);
    sensors::Device invalid { 0, "", sensors::Device::Type::CPUThread, 0, 0 };
    CHECK(sensors::getLoad(invalid) == sensors::error::code);
}

int main(int argc, char** argv)
{
    return doctest::Context(argc, argv).run();
}
