cmake_minimum_required(VERSION 3.14...3.20)

project(RPO_projekt
        DESCRIPTION ""
        LANGUAGES CXX
)

enable_testing()

add_executable(${PROJECT_NAME}
    main.cpp
    src/sensors/win_sensors.cpp
    src/sensors/linux_sensors.cpp
)

add_executable(Tests 
    tests/tests.cpp
    src/sensors/linux_sensors.cpp
    src/sensors/win_sensors.cpp
)
add_test(NAME Tests COMMAND Tests)


find_package(Qt5 COMPONENTS Core REQUIRED)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    message("-- DEBUG MODE")
    target_compile_definitions(${PROJECT_NAME} PUBLIC LLOG_ENABLED LLOG_COLORS_ENABLED)
    target_compile_definitions(Tests PUBLIC LLOG_ENABLED LLOG_COLORS_ENABLED)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

file(DOWNLOAD
    https://raw.githubusercontent.com/LiamMesarec/LLOG/main/llog.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/LLOG/llog.hpp
    SHOW_PROGRESS
)

include_directories(SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_compile_features(Tests PUBLIC cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)

if(UNIX)
    target_link_libraries(${PROJECT_NAME} PUBLIC sensors)
    target_link_libraries(Tests PUBLIC sensors)
endif(UNIX)

target_link_libraries(${PROJECT_NAME} PUBLIC Qt::Core)
target_link_libraries(Tests PUBLIC Qt::Core)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR "clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -std=c++20 -Wall -Wextra -Wpedantic -O2)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
endif()
