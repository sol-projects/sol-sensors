#include "shared/csv/csv.hpp" 
#include <fstream>
#include <chrono>
#include <ctime>
#include <fstream>
#include <LLOG/llog.hpp>

namespace csv
{
    Csv::Csv(const sensors::Device& device, int interval, MeasurementType measurementType)
        : m_measurementType(measurementType), m_removeFileOnDestruct(false)
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

        if(!file)
        {
            llog::Print(llog::pt::error, "Cannot open file", m_fileName, llog::Location());
        }

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


    std::vector<int> Csv::getLastnData(int n, int column)
    {
        if(column > 2)
        {
            llog::Print(llog::pt::error, "Invalid column! Can only access columns 1 or 2 in file", m_fileName);
            return {};
        }
        
        if(column == 2 && m_measurementType != MeasurementType::Both)
        {

            llog::Print(llog::pt::error, "Invalid column! Can only access column 1.", m_fileName);
            return {};
        }

        std::ifstream file(m_fileName);

        std::vector<int> data;
        data.reserve(n);

        file.seekg(0, std::ios_base::end);
        std::string line;
        for(int i = 0; i < n; i++)
        {
            char c = 0;
            while(c != '\n')
            {
                file.seekg(-2, std::ios_base::cur);

                if(static_cast<int>(file.tellg()) <= 1)
                {
                    file.seekg(0);
                    break;
                }
                
                file.get(c);
            }
            std::getline(file, line);
            file.seekg(-line.size() - 2, std::ios_base::cur);
            
            if(column == 1)
            {
                data.push_back(std::stoi(line.substr(0, line.find(';'))));
            }
            else if(column == 2)
            {
                int pos = line.find(';');
                auto str = line.substr(pos + 1);
                str = str.substr(0, str.find(';'));
                data.push_back(std::stoi(str));
            }

        }

        return data; 
    }
    
    void Csv::removeFileOnDestruct()
    {
        m_removeFileOnDestruct = true;
    }

    Csv::~Csv()
    {
        if(m_removeFileOnDestruct)
        {
            std::filesystem::remove(m_fileName);
        }
    }
}
