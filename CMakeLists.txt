cmake_minimum_required(VERSION 3.18...3.20)

project(RPO_projekt
        DESCRIPTION ""
        LANGUAGES CXX
)

option(BUILD_GUI "BUILD_GUI" ON)
option(BUILD_NOGUI "BUILD_NOGUI" ON)
option(BUILD_TESTS "BUILD_TESTS" ON)

if(BUILD_TESTS OR BUILD_GUI OR BUILD_NOGUI)
    include(FetchContent)
    find_package(Git REQUIRED)
endif()

add_compile_definitions(LLOG_ENABLED LLOG_COLORS_ENABLED)

if(BUILD_TESTS)
    add_compile_definitions(DOCTEST_CONFIG_IMPLEMENT)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR "clang")
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4 /WX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(libraries)
add_subdirectory(shared)
add_subdirectory(sensors)

if(BUILD_TESTS)
    add_definitions(-DTESTS)
    add_subdirectory(tests)
endif(BUILD_TESTS)

if(BUILD_GUI)
    add_definitions(-DGUI)
    add_subdirectory(gui)
endif(BUILD_GUI)

if(BUILD_NOGUI)
    add_definitions(-DNOGUI)
    add_subdirectory(nogui)
endif(BUILD_NOGUI)

add_executable(${PROJECT_NAME}
    main.cpp
)

if(BUILD_NOGUI)
    if(BUILD_GUI)
        target_link_libraries(${PROJECT_NAME} PUBLIC tests nogui gui)
    else()
        target_link_libraries(${PROJECT_NAME} PUBLIC tests nogui)
    endif()
else()
    target_link_libraries(${PROJECT_NAME} PUBLIC tests gui)
endif()

file(DOWNLOAD
    https://raw.githubusercontent.com/LiamMesarec/LLOG/main/llog.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/LLOG/llog.hpp
    SHOW_PROGRESS
)
