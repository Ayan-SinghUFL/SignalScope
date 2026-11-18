cmake_minimum_required(VERSION 3.22)
project(SignalScope)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

# Set this OFF to build only the KD-Tree test and benchmark without
# downloading SFML:  cmake -S . -B build -DSIGNALSCOPE_BUILD_APP=OFF
option(SIGNALSCOPE_BUILD_APP "Build the SFML map application" ON)

if(SIGNALSCOPE_BUILD_APP)
    include(FetchContent)
    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        2.6.1
    )

    set(SFML_BUILD_EXAMPLES OFF CACHE INTERNAL "")
    set(SFML_BUILD_DOC OFF CACHE INTERNAL "")

    FetchContent_MakeAvailable(SFML)
    set(SOURCES
        src/main.cpp
        src/QuadTree.cpp
        src/KDTree.cpp
        src/MathUtils.cpp
    )
    add_executable(SignalScope ${SOURCES})
    target_include_directories(SignalScope PRIVATE include)
    target_link_libraries(SignalScope PRIVATE sfml-graphics sfml-window sfml-system)
endif()

# KD-Tree + RF math tests (no SFML needed)
add_executable(kdtree_test tests/kdtree_test.cpp src/KDTree.cpp src/MathUtils.cpp)
target_include_directories(kdtree_test PRIVATE include)

# KD-Tree vs. brute-force nearest-neighbor benchmark (no SFML needed)
#   ./benchmark [numTowers] [numQueries]    e.g.  ./benchmark 100000 1000
add_executable(benchmark tests/benchmark.cpp src/KDTree.cpp src/MathUtils.cpp)
target_include_directories(benchmark PRIVATE include)
