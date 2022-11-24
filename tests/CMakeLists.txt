add_executable(tests
    tests.cpp
)

add_test(NAME tests COMMAND tests)

target_link_libraries(tests PUBLIC sensors)
