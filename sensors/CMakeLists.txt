add_library(sensors
        src/sensors/win_sensors.cpp
        src/sensors/linux_sensors.cpp
)

target_include_directories(sensors PUBLIC
        include
)

file(DOWNLOAD
    https://raw.githubusercontent.com/LiamMesarec/LLOG/main/llog.hpp
    ${CMAKE_BINARY_DIR}/LLOG/llog.hpp
    SHOW_PROGRESS
)
