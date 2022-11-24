add_library(sensors
        src/sensors/win_sensors.cpp
        src/sensors/linux_sensors.cpp
)

target_include_directories(sensors PUBLIC
        include
)
