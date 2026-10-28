add_library(tests INTERFACE)

set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libraries)

file(DOWNLOAD
    https://raw.githubusercontent.com/doctest/doctest/master/doctest/doctest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/DOCTEST/doctest.h
    SHOW_PROGRESS
)

target_link_libraries(tests INTERFACE sensors shared)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(linux_backend_check linux_backend.cpp)
    target_include_directories(linux_backend_check PRIVATE ../sensors/include)
    add_test(NAME linux_backend COMMAND linux_backend_check)
endif()

if(WIN32)
    add_executable(windows_backend_check windows_backend.cpp)
    target_include_directories(windows_backend_check PRIVATE ../sensors/include)
    target_compile_definitions(windows_backend_check PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(windows_backend_check PRIVATE pdh dxgi gdi32 advapi32)
    add_test(NAME windows_backend COMMAND windows_backend_check)
endif()
