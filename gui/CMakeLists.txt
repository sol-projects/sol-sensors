if(Qt6_FOUND)
    find_package(Qt6 COMPONENTS Core Widgets PrintSupport REQUIRED)
    message("-- Using Qt6")
endif()

if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Core Widgets PrintSupport REQUIRED)
    message("-- Using Qt5")
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(UI_FILES src/gui/mainwindow.ui src/gui/settings.ui)
qt5_wrap_ui(UI_SOURCES ${UI_FILES})

add_library(gui
    src/gui/mainwindow.cpp
    src/gui/settings.cpp
    src/gui/axistag.cpp
    src/gui/qcustomplot.cpp
    ${UI_SOURCES}
)

target_include_directories(gui PUBLIC
        include
)

target_link_libraries(gui PUBLIC sensors Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::PrintSupport)
