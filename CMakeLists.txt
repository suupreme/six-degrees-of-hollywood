cmake_minimum_required(VERSION 3.16)
project(SixDegreesOfHollywood LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Everything except main.cpp, shared by the program and the tests.
add_library(six_degrees_core STATIC
    src/Graph.cpp
    src/bfs.cpp
    src/loader.cpp
    src/bidirectional.cpp
    src/lookup.cpp
    src/benchmark.cpp
)
target_include_directories(six_degrees_core PUBLIC src)

if(MSVC)
    target_compile_options(six_degrees_core PUBLIC /W4)
else()
    target_compile_options(six_degrees_core PUBLIC -Wall -Wextra)
endif()

add_executable(six_degrees src/main.cpp)
target_link_libraries(six_degrees PRIVATE six_degrees_core)

add_executable(tests tests/test_main.cpp)
target_link_libraries(tests PRIVATE six_degrees_core)
# Lets the tests find the sample data no matter where they are run from.
target_compile_definitions(tests PRIVATE
    SAMPLE_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/sample")

enable_testing()
add_test(NAME tests COMMAND tests)
