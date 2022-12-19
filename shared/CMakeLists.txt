add_library(shared
    src/shared/config/config.cpp
    src/shared/csv/csv.cpp
)

set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libraries)

target_include_directories(shared PUBLIC
        include
)

target_link_libraries(shared PUBLIC sensors)
