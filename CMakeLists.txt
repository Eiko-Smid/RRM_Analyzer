
cmake_minimum_required(VERSION 3.16)

project(InMessRrmAnalyzer LANGUAGES CXX)

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------
# Parser / analysis library
# --------------------------------------------------
add_library(rrm_core
    src/rrm_log_parser.cpp
    src/string_helpers.cpp
)

target_include_directories(rrm_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Compiler warnings
if(MSVC)
    target_compile_options(rrm_core PRIVATE /W4)
else()
    target_compile_options(rrm_core PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

# --------------------------------------------------
# Executable
# --------------------------------------------------
# Add main exe
add_executable(rrm_analyzer
    src/main.cpp
)

target_link_libraries(rrm_analyzer
    PRIVATE rrm_core
)

# Add test exe for testing code
add_executable(test_log_parser
    tests/test_log_parser.cpp
)

target_link_libraries(test_log_parser
    PRIVATE rrm_core
)