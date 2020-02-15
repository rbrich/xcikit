cmake_minimum_required(VERSION 3.9)
project(PackageTest CXX)

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
find_package(benchmark REQUIRED)

enable_testing()
add_executable(example example.cpp)
target_link_libraries(example benchmark::benchmark)
