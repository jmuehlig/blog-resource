cmake_minimum_required(VERSION 3.20)
project(02_measure_profiling)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -DNDEBUG")
set(CMAKE_BUILD_TYPE RELEASE)

include(ExternalProject)
ExternalProject_Add(
        perf-cpp-external
        GIT_REPOSITORY "https://github.com/jmuehlig/perf-cpp"
        GIT_TAG "v0.14-dev"
        PREFIX "lib/perf-cpp"
        INSTALL_COMMAND cmake -E echo ""
)

ExternalProject_Add(
        tabulate-external
        GIT_REPOSITORY "https://github.com/p-ranav/tabulate"
        GIT_TAG "master"
        PREFIX "lib/tabulate"
        INSTALL_COMMAND cmake -E echo ""
)

include_directories(src/ lib/perf-cpp/src/perf-cpp-external/include lib/tabulate/src/tabulate-external/include)
link_directories(lib/perf-cpp/src/perf-cpp-external-build)

add_executable(random-access-bench src/main.cpp src/result.cpp)
target_link_libraries(random-access-bench perf-cpp)

add_custom_target(plots
    COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/plot.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Plotting all result*.csv files"
)

add_custom_target(plots-overhead
    COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/plot_overhead.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Plotting instruction overhead comparison"
)