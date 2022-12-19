cmake_minimum_required(VERSION 3.14)
project(examples LANGUAGES CXX)

include(FetchContent)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FetchContent_Declare(
    sensors
    URL https://github.com/sol-projects/rpo/files/10262169/sensors.zip
    GIT_TAG main
    DOWNLOAD_DIR sensors
)
FetchContent_MakeAvailable(sensors)

add_executable(${PROJECT_NAME} main3.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC sensors)
