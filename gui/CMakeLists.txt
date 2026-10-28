find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets PrintSupport)
if(QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 COMPONENTS Core Widgets PrintSupport REQUIRED)
    message("-- Using Qt6")
endif()

if(QT_VERSION_MAJOR EQUAL 5)
    find_package(Qt5 COMPONENTS Core Widgets PrintSupport REQUIRED)
    message("-- Using Qt5")
endif()

set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libraries)

include(FetchContent)

FetchContent_Declare(
    qcustomplot
    URL https://www.qcustomplot.com/release/2.1.1/QCustomPlot-source.tar.gz
    DOWNLOAD_NO_EXTRACT true
    DOWNLOAD_DIR QCUSTOMPLOT
)
FetchContent_MakeAvailable(qcustomplot)

file(ARCHIVE_EXTRACT INPUT ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/qcustomplot-src/QCustomPlot-source.tar.gz DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/QCUSTOMPLOT)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_library(gui
    src/gui/gui.cpp
    src/gui/MainWindow.cpp
    src/gui/AxisTag.cpp
    src/gui/mainwindow.ui
    ../libraries/QCUSTOMPLOT/qcustomplot-source/qcustomplot.cpp
    include/gui/MainWindow.hpp
    include/gui/AxisTag.hpp
    ../libraries/QCUSTOMPLOT/qcustomplot-source/qcustomplot.h
)

target_include_directories(gui PUBLIC
        include
        ../libraries/QCUSTOMPLOT/qcustomplot-source
)

target_link_libraries(gui PUBLIC sensors Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::PrintSupport)

if(BUILD_TESTS)
    add_executable(dashboard_check ../tests/dashboard.cpp)
    target_link_libraries(dashboard_check PRIVATE gui)
    add_test(NAME dashboard_controls COMMAND ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=offscreen $<TARGET_FILE:dashboard_check>)

    add_executable(dashboard_live_check ../tests/dashboard.cpp)
    target_compile_definitions(dashboard_live_check PRIVATE LIVE_SENSORS)
    target_link_libraries(dashboard_live_check PRIVATE gui)
    add_test(NAME dashboard_live COMMAND ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=offscreen
        $<TARGET_FILE:dashboard_live_check> ${CMAKE_BINARY_DIR}/dashboard.png ${CMAKE_BINARY_DIR}/dashboard-narrow.png ${CMAKE_BINARY_DIR}/dashboard-full.png)
endif()
