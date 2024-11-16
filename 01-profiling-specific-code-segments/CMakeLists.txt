cmake_minimum_required(VERSION 3.28)
project(random-access-bench)
include(ExternalProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -DNDEBUG")
set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

ExternalProject_Add(
    perf-cpp-external
    GIT_REPOSITORY "https://github.com/jmuehlig/perf-cpp"
    GIT_TAG "v0.8.3"
    PREFIX "lib/perf-cpp"
    INSTALL_COMMAND cmake -E echo ""
)

ExternalProject_Add(
    argparse-external
    GIT_REPOSITORY "https://github.com/p-ranav/argparse"
    GIT_TAG "v2.9"
    PREFIX "lib/argparse"
    INSTALL_COMMAND cmake -E echo ""
)

include_directories(lib/perf-cpp/src/perf-cpp-external/include lib/argparse/src/argparse-external/include)
link_directories(lib/perf-cpp/src/perf-cpp-external-build)

add_executable(random-access-bench main.cpp)
target_link_libraries(random-access-bench perf-cpp)