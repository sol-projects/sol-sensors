cmake_minimum_required(VERSION 3.18...3.20)

enable_testing()

project(RPO_projekt
        DESCRIPTION ""
        LANGUAGES CXX
)

add_compile_definitions(LLOG_ENABLED LLOG_COLORS_ENABLED)

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
add_subdirectory(sensors)
add_subdirectory(tests)
add_subdirectory(gui)
add_subdirectory(nogui)

add_executable(${PROJECT_NAME}
    main.cpp
)

file(DOWNLOAD
    https://raw.githubusercontent.com/LiamMesarec/LLOG/main/llog.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/LLOG/llog.hpp
    SHOW_PROGRESS
)
