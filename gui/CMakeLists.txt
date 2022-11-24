add_library(gui
    src/gui/delete_me.cpp
)

target_include_directories(gui PUBLIC
        include
)

if(Qt6_FOUND)
    find_package(Qt6 COMPONENTS Core REQUIRED)
    message("-- Using Qt6")
endif()

if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Core REQUIRED)
    message("-- Using Qt5")
endif()

target_link_libraries(gui PUBLIC sensors Qt::Core)
