add_library(server
    src/server/server.cpp
)

set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libraries)

target_include_directories(server PUBLIC
        include
)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG master
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
)

FetchContent_MakeAvailable(asio)

target_link_libraries(server PUBLIC sensors shared)
target_include_directories(server SYSTEM PRIVATE ${asio_SOURCE_DIR}/asio/include)

target_compile_definitions(server PUBLIC ASIO_STANDALONE PURIFY)

