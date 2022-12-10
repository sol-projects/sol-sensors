#include "nogui/nogui.hpp"
#include "sensors/sensors.hpp"
#include <algorithm>
#include <cctype>
#include <string>

namespace nogui
{
    namespace
    {
        cag_option_context context;

        std::vector<sensors::Device> getDevices(std::string& devicesToMeasure)
        {
            std::transform(std::begin(devicesToMeasure), std::end(devicesToMeasure), std::begin(devicesToMeasure), ::tolower);

            if(devicesToMeasure == "all")
            {
                return sensors::getDevices(sensors::Device::Type::Any);
            }

            std::vector<sensors::Device> devices;

            auto addToDevicesIfExists = [&devicesToMeasure, &devices](const std::string& deviceName, sensors::Device::Type type)
            {
                if(devicesToMeasure.find(deviceName) != std::string::npos)
                {
                    auto newDevices = sensors::getDevices(type);
                    devices.insert(std::end(devices), std::begin(newDevices), std::end(newDevices));
                }
            };

            addToDevicesIfExists("cpu", sensors::Device::Type::CPU);
            addToDevicesIfExists("ram", sensors::Device::Type::RAM);
            addToDevicesIfExists("vram", sensors::Device::Type::VRAM);
            addToDevicesIfExists("gpu", sensors::Device::Type::GPU);

            return devices;
        }

        void runProgramAtInterval(bool temperature, bool load, int interval, std::vector<sensors::Device> devices, bool file)
        {

        }
    }

    void run(int argc, char* argv[])
    {
        OptionFlags optionFlags{};
        cag_option_prepare(&context, options, CAG_ARRAY_SIZE(options), argc, argv);
        while (cag_option_fetch(&context))
        {
            auto identifier = cag_option_get(&context);
            switch (identifier)
            {
                case 'i':
                    optionFlags.interval = std::stoi(cag_option_get_value(&context));
                    break;
                case 't':
                    optionFlags.temperature = true;
                    break;
                case 'l':
                    optionFlags.load = true;
                    break;
                case 'd':
                    optionFlags.devices = cag_option_get_value(&context);
                    break;
                case 'f':
                    optionFlags.file = true;
                    break;
                case 'h':
                    cag_option_print(options, CAG_ARRAY_SIZE(options), stdout);
                    break;
            }
        }

        if(optionFlags.interval != 0)
        {
            runProgramAtInterval(optionFlags.temperature, optionFlags.load, optionFlags.interval, getDevices(optionFlags.devices), optionFlags.file);
        }
    }
}
