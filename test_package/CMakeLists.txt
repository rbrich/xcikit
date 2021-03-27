cmake_minimum_required(VERSION 3.13)
project(PackageTest CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")

# Setup RPATH
if (APPLE)
    set(CMAKE_INSTALL_RPATH "@executable_path/../lib")
else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
find_package(xcikit CONFIG REQUIRED)

add_executable(example example.cpp)
target_link_libraries(example xcikit::xci-core)
install(TARGETS example DESTINATION bin)
