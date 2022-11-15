#!/bin/bash
clang-format -i --files=./clang-format src/sensors/win_sensors.cpp
clang-format -i --files=./clang-format src/sensors/linux_sensors.cpp
clang-format -i --files=./clang-format include/sensors/Device.hpp
clang-format -i --files=./clang-format include/sensors/sensors.hpp
