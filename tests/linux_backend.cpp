#include "../sensors/src/sensors/linux_sensors.cpp"
#include <DOCTEST/doctest.h>

TEST_CASE("AMD sysfs discovery and measurements")
{
    auto root = std::filesystem::temp_directory_path() / ("sol-sensors-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup
    {
        std::filesystem::path path;
        ~Cleanup() { std::filesystem::remove_all(path); }
    } cleanup { root };
    auto write = [&](const std::filesystem::path& path, const std::string& value) {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream(path) << value;
    };
    auto path = root / "card3/device";
    write(path / "vendor", "0x1002\n");
    write(path / "product_name", "Test Radeon\n");
    write(path / "gpu_busy_percent", "37\n");
    write(path / "mem_info_vram_used", "2500000000\n");
    write(path / "hwmon/hwmon7/temp1_input", "45670\n");
    write(path / "hwmon/hwmon7/temp2_label", "junction\n");
    write(path / "hwmon/hwmon7/temp2_input", "77000\n");
    write(path / "hwmon/hwmon7/temp3_label", "mem\n");
    write(path / "hwmon/hwmon7/temp3_input", "62500\n");
    write(root / "card8/device/vendor", "0x1002\n");
    write(root / "card8/device/gpu_busy_percent", "9\n");
    write(root / "card2/device/vendor", "0x8086\n");
    write(root / "card3-DP-1/device/vendor", "0x1002\n");
    auto gpus = AMDinfo(root);
    REQUIRE(gpus.size() == 2);
    auto found = std::find_if(gpus.begin(), gpus.end(), [](const auto& gpu) { return gpu.name == "Test Radeon (card3)"; });
    REQUIRE(found != gpus.end());
    auto gpu = *found;
    auto vram = gpu;
    vram.type = sensors::Device::Type::VRAM;
    vram.name += " VRAM";
    CHECK(sensors::getLoad(gpu, 2) == 3700);
    CHECK(sensors::getTemp(gpu, 2) == 4567);
    CHECK(sensors::getTemp(gpu, 3) == 45670);
    CHECK(sensors::getLoad(vram, 2) == 250);
    CHECK(sensors::getTemp(vram, 2) == 6250);
    CHECK(sensors::getLoad(gpu, -1) == sensors::error::code);
    CHECK(sensors::getTemp(gpu, 7) == sensors::error::code);
    CHECK(sensors::getLoad(vram, 10) == sensors::error::code);
    write(path / "gpu_busy_percent", "bad data");
    CHECK(sensors::getLoad(gpu, 2) == sensors::error::code);
    write(path / "gpu_busy_percent", "-5");
    CHECK(sensors::getLoad(gpu, 2) == sensors::error::code);
    std::filesystem::remove(path / "hwmon/hwmon7/temp3_label");
    CHECK(sensors::getTemp(vram, 2) == sensors::error::code);
    std::filesystem::remove_all(path / "hwmon");
    CHECK(sensors::getTemp(gpu, 2) == sensors::error::code);
    CHECK(AMDinfo(root / "missing").empty());
    CHECK(scaledValue(1e20, 2) == sensors::error::code);
}

int main(int argc, char** argv)
{
    return doctest::Context(argc, argv).run();
}
