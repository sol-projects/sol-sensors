#pragma once
#include <filesystem>
#include "sensors/Device.hpp"
#include <vector>

namespace csv
{
    enum class MeasurementType
    {
        Load, Temperature, Both
    };

    class Csv
    {
    public:
        Csv(const sensors::Device& device, int interval, MeasurementType measurementType);
        void add(const sensors::Device& device);
        std::vector<int> getLastnData(int n);
    private:
        std::filesystem::path m_fileName;
        MeasurementType m_measurementType;
    };
}
