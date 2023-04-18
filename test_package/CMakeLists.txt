cmake_minimum_required(VERSION 3.16)
project(PackageTest CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

find_package(xcikit CONFIG REQUIRED)

add_executable(example example.cpp)
target_link_libraries(example PRIVATE xcikit::xci-core)
target_compile_features(example PRIVATE cxx_std_20)
