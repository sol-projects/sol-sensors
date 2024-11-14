add_library(sensors
        src/sensors/win_sensors.cpp
        src/sensors/linux_sensors.cpp
        src/sensors/mac_sensors.cpp
)

target_include_directories(sensors PUBLIC
        include
        ${CMAKE_BINARY_DIR}
)

file(DOWNLOAD
    https://raw.githubusercontent.com/LiamMesarec/LLOG/main/llog.hpp
    ${CMAKE_BINARY_DIR}/LLOG/llog.hpp
    SHOW_PROGRESS
)

target_compile_features(sensors PUBLIC cxx_std_20)
