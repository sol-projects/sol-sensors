cmake_minimum_required(VERSION 3.18...3.20)

project(RPO_projekt
        DESCRIPTION ""
        LANGUAGES CXX
)

include(FetchContent)
find_package(Git REQUIRED)

add_compile_definitions(LLOG_ENABLED LLOG_COLORS_ENABLED DOCTEST_CONFIG_IMPLEMENT)

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
add_subdirectory(tests)
add_subdirectory(gui)
add_subdirectory(nogui)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC tests)
file(DOWNLOAD
    https://raw.githubusercontent.com/LiamMesarec/LLOG/main/llog.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/LLOG/llog.hpp
    SHOW_PROGRESS
)
