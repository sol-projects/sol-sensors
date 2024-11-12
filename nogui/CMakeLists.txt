add_library(nogui
    src/nogui/nogui.cpp
)

set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libraries)

target_include_directories(nogui PUBLIC
        include
)

FetchContent_Declare(
    cargs
    GIT_REPOSITORY https://github.com/likle/cargs.git
    GIT_TAG stable
)

FetchContent_MakeAvailable(cargs)
target_link_libraries(nogui PUBLIC cargs sensors shared)
