add_library(shared
    src/shared/config/config.cpp
)

target_include_directories(shared PUBLIC
        include
)
