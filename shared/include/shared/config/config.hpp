#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace config
{
    const std::filesystem::path path = "config.conf";

    struct Option
    {
        std::string name;
        std::string value;
    };

    void init();
    void add(const Option& option);
    void set(const std::string& name, const std::string& value);
    void set(const Option& option);
    std::string get();
    Option get(const std::string& name);
    void remove(const Option& option);
    bool exists(const std::string& name);
    bool saved();
    void save();
    void autosave(bool set);
}
