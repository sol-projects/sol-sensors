#!/bin/bash
clang-format -i --files=./clang-format sensors/src/sensors/win_sensors.cpp
clang-format -i --files=./clang-format sensors/src/sensors/linux_sensors.cpp
clang-format -i --files=./clang-format sensors/include/sensors/Device.hpp
clang-format -i --files=./clang-format sensors/include/sensors/sensors.hpp
clang-format -i --files=./clang-format tests/config.hpp
clang-format -i --files=./clang-format tests/sensors.hpp
clang-format -i --files=./clang-format shared/include/shared/config/config.hpp
clang-format -i --files=./clang-format shared/src/shared/config/config.cpp
clang-format -i --files=./clang-format main.cpp
