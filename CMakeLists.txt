
cmake_minimum_required(VERSION 3.16)

project(InMessRrmAnalyzer LANGUAGES CXX)

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Qt libraries
find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)

# --------------------------------------------------
# Parser / analysis library
# --------------------------------------------------

add_library(rrm_core
    src/rrm_log_parser.cpp
    src/string_helpers.cpp
    src/measurement_analyzer.cpp
)

target_include_directories(rrm_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Handle compiler warnings
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

# Define main exe and gui
add_executable(rrm_analyzer
    src/main.cpp
    src/gui/main_window.cpp           
    src/gui/measurement_widget.cpp    
)

target_link_libraries(rrm_analyzer
    PRIVATE
        rrm_core
        Qt6::Widgets
        Qt6::Charts
)

# Define test exe
# Testing log parser
add_executable(test_log_parser
    tests/test_log_parser.cpp
    
)

target_link_libraries(test_log_parser
    PRIVATE rrm_core
)

# Testing measurement analyzer
add_executable(test_measurement_analyzer
    tests/test_measurement_analyzer.cpp
)

target_link_libraries(test_measurement_analyzer
    PRIVATE rrm_core
)

