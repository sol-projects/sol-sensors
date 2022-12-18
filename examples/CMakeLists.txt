cmake_minimum_required(VERSION 3.14)
project(examples LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    sensors
    URL https://github.com/sol-projects/rpo/files/10252986/sensors.tar.gz
    GIT_TAG main
    DOWNLOAD_DIR sensors
)

FetchContent_MakeAvailable(sensors)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC sensors)
