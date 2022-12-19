#pragma once
#include "sensors/Device.hpp"
#include <filesystem>
#include <vector>

namespace csv
{
    enum class MeasurementType
    {
        Load,
        Temperature,
        Both
    };

    class Csv
    {
    public:
        Csv(const sensors::Device& device, int interval, MeasurementType measurementType);
        ~Csv();
        void add(const sensors::Device& device);
        std::vector<int> getLastnData(int n, int column = 1);
        void removeFileOnDestruct();

    private:
        std::filesystem::path m_fileName;
        MeasurementType m_measurementType;
        bool m_removeFileOnDestruct;
    };
}
