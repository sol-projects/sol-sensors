#include "nogui/nogui.hpp"
#include "sensors/sensors.hpp"
#include "shared/config/config.hpp"
#include "shared/csv/csv.hpp"
#include <LLOG/llog.hpp>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <string>
#include <thread>

namespace nogui
{
    namespace
    {
        std::vector<sensors::Device> getDevices(std::string& devicesToMeasure)
        {
            std::transform(std::begin(devicesToMeasure), std::end(devicesToMeasure), std::begin(devicesToMeasure), ::tolower);

            if (devicesToMeasure == "all")
            {
                return sensors::getDevices(sensors::Device::Type::Any);
            }

            std::vector<sensors::Device> devices;

            auto addToDevicesIfExists = [&devicesToMeasure, &devices](const std::string& deviceName, sensors::Device::Type type) {
                if (auto found = devicesToMeasure.find(deviceName); found != std::string::npos)
                {
                    auto newDevices = sensors::getDevices(type);
                    devices.insert(std::end(devices), std::begin(newDevices), std::end(newDevices));
                    devicesToMeasure.erase(found, deviceName.size());
                }
            };

            addToDevicesIfExists("cput", sensors::Device::Type::CPUThread);
            addToDevicesIfExists("cpu", sensors::Device::Type::CPU);
            addToDevicesIfExists("vram", sensors::Device::Type::VRAM);
            addToDevicesIfExists("ram", sensors::Device::Type::RAM);
            addToDevicesIfExists("gpu", sensors::Device::Type::GPU);

            return devices;
        }

        void runProgramAtInterval(bool temperature, bool load, int interval, std::vector<sensors::Device> devices, bool fileFlag, const std::string& filepath, int accuracy)
        {
            using namespace std::chrono_literals;

            if (devices.empty())
            {
                llog::Print(llog::pt::error, "No devices found.");
                return;
            }

            csv::MeasurementType measurementType;
            if (temperature && load)
            {
                measurementType = csv::MeasurementType::Both;
            }
            else if (temperature)
            {
                measurementType = csv::MeasurementType::Temperature;
            }
            else if (load)
            {
                measurementType = csv::MeasurementType::Load;
            }
            else
            {
                llog::Print(llog::pt::error, "Must use flags -t or -l to choose which temperatures to measure.");
                return;
            }

            std::vector<csv::Csv> files;
            if (fileFlag)
            {
                for (const auto& device : devices)
                {
                    files.emplace_back(device, interval, measurementType, filepath);
                }
            }
            else if (std::size(devices) > 1 || (load && temperature))
            {
                llog::Print(llog::pt::error, "You can only use a maximum of 1 device and 1 type of measurement if you're not combining with the -f option.");
                return;
            }

            for (;;)
            {
                auto intervalStartTime = std::chrono::high_resolution_clock::now();
                for (auto i = 0ULL; i < std::size(devices); i++)
                {
                    auto& device = devices.at(i);

                    if (temperature)
                    {
                        device.temperature = sensors::getTemp(device, accuracy);
                    }

                    if (load)
                    {
                        device.load = sensors::getLoad(device, accuracy);
                    }

                    if (fileFlag)
                    {
                        auto& file = files.at(i);
                        file.add(device);
                    }
                    else
                    {
                        if (temperature)
                        {
                            std::cout << device.temperature << '\n';
                        }
                        else if (load)
                        {
                            std::cout << device.load << '\n';
                        }
                    }
                }

                auto intervalEndTime = std::chrono::high_resolution_clock::now();

                auto intervalCycleTime = std::chrono::duration_cast<std::chrono::microseconds>(intervalEndTime - intervalStartTime);

                if (intervalCycleTime.count() < 0)
                {
                    llog::Print(llog::pt::error, "Improper interval, must be at least" - 1000 * intervalCycleTime.count(), "longer");
                    return;
                }

                std::this_thread::sleep_for(std::chrono::microseconds(interval * 1000 - intervalCycleTime.count()));
            }
        }

        OptionFlags parse(int argc, char* argv[])
        {
            OptionFlags optionFlags {};

            cag_option_context context;
            cag_option_prepare(&context, options, CAG_ARRAY_SIZE(options), argc, argv);
            while (cag_option_fetch(&context))
            {
                auto identifier = cag_option_get(&context);
                switch (identifier)
                {
                    case 'a':
                        optionFlags.accuracy = std::stoi(cag_option_get_value(&context));
                        break;
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
                    case 'p':
                        optionFlags.path = cag_option_get_value(&context);
                        break;
                    case 's':
                        optionFlags.startMeasurement = cag_option_get_value(&context);
                        break;
                    case 'e':
                        optionFlags.endMeasurement = cag_option_get_value(&context);
                        break;
                    case 'r':
                        optionFlags.run = true;
                        break;
                    case 'h':
                        cag_option_print(options, CAG_ARRAY_SIZE(options), stdout);
                        std::exit(1);
                }
            }

            return optionFlags;
        }
    }

    void run(int argc, char* argv[])
    {
        auto optionFlags = parse(argc, argv);

        if (optionFlags.run)
        {
            struct MeasurementInfo
            {
                std::string command;
                std::chrono::time_point<std::chrono::system_clock> start;
                std::chrono::time_point<std::chrono::system_clock> end;
            };

            std::vector<MeasurementInfo> measurementInfos;

            std::ifstream file(config::path);
            std::string line;
            while (std::getline(file, line))
            {
                MeasurementInfo measurementInfo{};

                if (line.starts_with("measurement"))
                {
                    for(auto i = line.find(':') + 1; i < line.size() && line.at(i) != '\n'; i++)
                    {
                        measurementInfo.command += line.at(i);
                    }

                    auto fromTime = [](const std::string& string)
                    {
                        std::string timeString;
                        for(auto i = string.find(':') + 1; i < string.size() && string.at(i) != '\n'; i++)
                        {
                            timeString += string.at(i);
                        }
                        std::istringstream ss(timeString);

                        std::tm tm = {};
                        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

                        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
                    };

                    std::getline(file, line);
                    measurementInfo.start = fromTime(line);
                    std::getline(file, line);
                    measurementInfo.end = fromTime(line);
                    measurementInfos.push_back(measurementInfo);
                }

                std::sort(std::begin(measurementInfos), std::end(measurementInfos), [](const auto& lhs, const auto& rhs) {
                        return std::chrono::duration_cast<std::chrono::seconds>(lhs.start.time_since_epoch()).count() < std::chrono::duration_cast<std::chrono::seconds>(rhs.end.time_since_epoch()).count();
                });

                while(!measurementInfos.empty())
                {
                    if(const auto measurementStartTime = std::chrono::duration_cast<std::chrono::seconds>(measurementInfos.at(0).start.time_since_epoch()).count(); measurementStartTime > std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count())
                    {
                        continue;
                    }
else
{
std::vector<char*> args;
std::string arg;
for(auto c : measurementInfos.at(0).command)
{
if(c==' ')
{
arg.clear();
}
else
{
arg += c;
}
}
char* carg = const_cast<char*>(arg.c_str());
args.push_back(carg);
                        std::thread runThread([&args]()
                        {
                            auto optionFlags = parse(args.size(), args.data());
                            runProgramAtInterval(optionFlags.temperature, optionFlags.load, optionFlags.interval, getDevices(optionFlags.devices), optionFlags.file, optionFlags.path, optionFlags.accuracy);
                        });

                        runThread.join();
                        measurementInfos.erase(std::begin(measurementInfos), std::begin(measurementInfos) + 1);
                    }
                }
            }
        }
        else if (optionFlags.interval != 0)
        {
            if (!optionFlags.startMeasurement.empty() && !optionFlags.endMeasurement.empty())
            {
                std::string args;
                for (int i = 0; i < argc; i++)
                {
                    if (argv[i][1] == 's' || argv[i][1] == 'e')
                    {
                        continue;
                    }

                    args += argv[i];
                    args += " ";
                }

                args.erase(args.size() - 1, 1);

                std::size_t index = 0;
                std::ifstream file(config::path);
                std::string lastMeasurement;

                std::string line;
                while (std::getline(file, line))
                {
                    if (line.starts_with("measurement"))
                    {
                        lastMeasurement = line;
                    }
                }

                if (!lastMeasurement.empty())
                {
                    lastMeasurement.erase(0, std::string("measurement").size());
                    std::string stringIndex;
                    for (auto c : lastMeasurement)
                    {
                        if (c == ':')
                        {
                            break;
                        }

                        stringIndex += c;
                    }

                    index = std::stoi(stringIndex) + 1;
                }

                config::Option measurement {
                    .name = "measurement" + std::to_string(index),
                    .value = args
                };

                config::Option startMeasurement {
                    .name = "start_measurement" + std::to_string(index),
                    .value = optionFlags.startMeasurement
                };

                config::Option endMeasurement {
                    .name = "end_measurement" + std::to_string(index),
                    .value = optionFlags.endMeasurement
                };

                config::add(measurement);
                config::set(measurement);
                config::add(startMeasurement);
                config::set(startMeasurement);
                config::add(endMeasurement);
                config::set(endMeasurement);

                config::save();
            }
            else
            {
                runProgramAtInterval(optionFlags.temperature, optionFlags.load, optionFlags.interval, getDevices(optionFlags.devices), optionFlags.file, optionFlags.path, optionFlags.accuracy);
            }
        }
    }
}
