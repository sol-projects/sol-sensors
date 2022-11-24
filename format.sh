#!/bin/bash
clang-format -i --files=./clang-format sensors/src/sensors/win_sensors.cpp
clang-format -i --files=./clang-format sensors/src/sensors/linux_sensors.cpp
clang-format -i --files=./clang-format sensors/include/sensors/Device.hpp
clang-format -i --files=./clang-format sensors/include/sensors/sensors.hpp
clang-format -i --files=./clang-format tests/tests.cpp
