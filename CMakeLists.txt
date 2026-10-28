cmake_minimum_required(VERSION 3.18...3.20)

project(SOL_SENSORS
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
    add_compile_definitions(DOCTEST_CONFIG_IMPLEMENT DOCTEST_COUNTER=__COUNTER__)
    enable_testing()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4 /utf-8)
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

target_link_libraries(${PROJECT_NAME} PRIVATE shared)

if(BUILD_TESTS)
    target_link_libraries(${PROJECT_NAME} PRIVATE tests)
    # The legacy console test calls exit(); check CLI help in a separate process.
    add_test(NAME sensors_tests COMMAND ${PROJECT_NAME} run_tests "--test-case-exclude=Testing the console application.")
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test-work)
    set_tests_properties(sensors_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test-work)
    if(BUILD_NOGUI)
        add_test(NAME console_help COMMAND ${PROJECT_NAME} -h)
        set_tests_properties(console_help PROPERTIES WILL_FAIL TRUE WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test-work)
    endif()
endif()

if(BUILD_NOGUI)
    if(BUILD_GUI)
        if(BUILD_TESTS)
            target_link_libraries(${PROJECT_NAME} PUBLIC tests nogui gui)
        else()
            target_link_libraries(${PROJECT_NAME} PUBLIC gui nogui)
        endif()
    elseif(BUILD_TESTS)
        target_link_libraries(${PROJECT_NAME} PUBLIC tests nogui)
    else()
        target_link_libraries(${PROJECT_NAME} PUBLIC nogui)
    endif()
elseif(BUILD_GUI)
    if(BUILD_TESTS)
        target_link_libraries(${PROJECT_NAME} PUBLIC tests gui)
    else()
        target_link_libraries(${PROJECT_NAME} PUBLIC gui)
    endif()
endif()

file(DOWNLOAD
    https://raw.githubusercontent.com/LiamMesarec/LLOG/main/llog.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/LLOG/llog.hpp
    SHOW_PROGRESS
)
