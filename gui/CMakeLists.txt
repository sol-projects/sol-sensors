if(Qt6_FOUND)
    find_package(Qt6 COMPONENTS Core Widgets PrintSupport REQUIRED)
    message("-- Using Qt6")
endif()

if(NOT Qt6_FOUND)
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
    src/gui/mainwindow.cpp
    src/gui/settings.cpp
    src/gui/settings.ui
    src/gui/mainwindow.ui
    src/gui/axistag.cpp
    ../libraries/QCUSTOMPLOT/qcustomplot-source/qcustomplot.cpp
    include/gui/settings.h
    include/gui/mainwindow.h
    include/gui/axistag.h
    ../libraries/QCUSTOMPLOT/qcustomplot-source/qcustomplot.h
)

target_include_directories(gui PUBLIC
        include
        ../libraries/QCUSTOMPLOT/qcustomplot-source
)

target_link_libraries(gui PUBLIC sensors Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::PrintSupport)
