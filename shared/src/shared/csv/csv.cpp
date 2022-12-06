#include "shared/csv/csv.hpp" 
#include <fstream>
#include <chrono>
#include <ctime>
#include <fstream>

namespace csv
{
    Csv::Csv(const sensors::Device& device, int interval, MeasurementType measurementType)
        : m_measurementType(measurementType)
    {
        std::string datetime(30, '\0');

        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        std::strftime(&datetime[0], std::size(datetime), "%Y_%m_%d_%H_%M_%S", std::localtime(&now));


        datetime.erase(datetime.find('\0'));
        std::string filename;
        filename += datetime + "_";
        filename += std::to_string(interval) + "_";
        filename += device.name + ".txt";

        m_fileName = filename;
        
        std::ofstream file(m_fileName);
        if(m_measurementType == MeasurementType::Both)
        {
            file << "Load;Temperature" << '\n';
        }
        else if(m_measurementType == MeasurementType::Load)
        {
            file << "Load" << '\n';
        }
        else if(m_measurementType == MeasurementType::Temperature)
        {
            file << "Temperature" << '\n';
        }
    }

    void Csv::add(const sensors::Device& device)
    {
        std::ofstream file(m_fileName, std::ios_base::app);

        if(m_measurementType == MeasurementType::Both)
        {
            file << device.load << ';' << device.temperature << '\n';
        }
        else if(m_measurementType == MeasurementType::Load)
        {
            file << device.load << '\n';
        }
        else if(m_measurementType == MeasurementType::Temperature)
        {
            file << device.temperature << '\n';
        }
    }


    std::vector<int> getLastnData(int n)
    {
        return {}; 
    }
}
