#include "shared/config/config.hpp"
#include <LLOG/llog.hpp>
#include <algorithm>
#include <fstream>
#include <iterator>

namespace config
{
    namespace
    {
        std::string configString;
        bool isAutosaving = false;
    }

    void init()
    {
        configString = "";
        Option cpuLinuxTempMeasurementMode {
            .name = "cpu_linux_temperature_measurement_mode",
            .value = "any",
        };

        Option cpuTempDecPrecision {
            .name = "cpu_temperature_decimal_precision",
            .value = "1"
        };

        add(cpuLinuxTempMeasurementMode);
        set(cpuLinuxTempMeasurementMode);
        add(cpuTempDecPrecision);
        set(cpuTempDecPrecision);
    }

    void add(const Option& option)
    {
        if (exists(option.name))
        {
            llog::Print(llog::pt::warning, "Option", option.name, "already exists, ignoring call to add it.");
            return;
        }

        configString += option.name + ": \n";

        if (isAutosaving)
        {
            save();
        }
    }

    void set(const std::string& name, const std::string& value)
    {
        set({ name, value });
    }

    void set(const Option& option)
    {
        auto optionEndPos = configString.find(option.name) + std::size(option.name) + 2;
        while (configString.at(optionEndPos) != '\n')
        {
            configString.erase(optionEndPos, 1);
        }

        configString.insert(optionEndPos, option.value);

        if (isAutosaving)
        {
            save();
        }
    }

    std::string get()
    {
        return configString;
    }

    Option get(const std::string& name)
    {
        auto optionEndPos = configString.find(name) + std::size(name) + 2;

        std::string value;
        for (auto i = optionEndPos; configString.at(i) != '\n'; i++)
        {
            value += configString.at(i);
        }

        return { name, value };
    }

    void remove(const Option& option)
    {
        auto optionPos = configString.find(option.name);
        for (auto i = optionPos; configString.at(i) != '\n'; i++)
        {
            configString.erase(i, 1);
        }

        if (isAutosaving)
        {
            save();
        }
    }

    bool exists(const std::string& name)
    {
        return configString.find(name) != std::string::npos;
    }

    bool saved()
    {
        std::ifstream file(path);

        char c;
        int i = 0;

        while (file >> std::noskipws >> c)
        {
            if (c != configString.at(i))
            {
                return false;
            }

            i++;
        }

        return true;
    }

    void save()
    {
        std::ofstream file(path);

        if (!file)
        {
            llog::Print(llog::pt::error, "Cannot open file:", path);
        }

        file << configString;
    }

    void autosave(bool set)
    {
        isAutosaving = set;
    }
}
