add_library(tests INTERFACE)

set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libraries)

file(DOWNLOAD
    https://raw.githubusercontent.com/doctest/doctest/master/doctest/doctest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/DOCTEST/doctest.h
    SHOW_PROGRESS
)

target_link_libraries(tests INTERFACE sensors shared)
