#include "shared/config/config.hpp"
#include <iterator>
#include <algorithm>
#include <fstream>
#include <LLOG/llog.hpp>

namespace config
{
    namespace
    {
        std::string configString;
    }

    void init()
    {
        configString = "";
        Option CpuLinuxTempMeasurementMode
        {
            .name = "cpu_linux_temperature_measurement_mode",
            .value = "any",
        };

        Option CpuTempDecPrecision
        {
            .name = "cpu_temperature_decimal_precision",
            .value = "1"
        };

        add(CpuLinuxTempMeasurementMode);
        set(CpuLinuxTempMeasurementMode);
        add(CpuTempDecPrecision);
        set(CpuTempDecPrecision);
    }

    void add(const Option& option)
    {
        if(exists(option.name))
        {
            llog::Print(llog::pt::warning, "Option", option.name, "already exists, ignoring call to add it.");
            return;
        }

        configString += option.name + ": \n";   
    }

    void set(const std::string& name, const std::string& value)
    {
        set({ name, value });
    }

    void set(const Option& option)
    {
        int optionEndPos = configString.find(option.name) + std::size(option.name) + 2;
        for(int i = optionEndPos; configString.at(i) != '\n'; i++)
        {
            configString.erase(i, 1);
        }

        configString.insert(optionEndPos, option.value);
    }

    Option get(const std::string& name)
    {
        int optionEndPos = configString.find(name) + std::size(name) + 2;

        std::string value;
        for(int i = optionEndPos; configString.at(i) != '\n'; i++)
        {
            value += configString.at(i);
        }

        return { name, value };
    }
    
    void remove(const Option& option)
    {

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
            if(c != configString.at(i))
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
        file << configString;
    }
}
