cmake_minimum_required(VERSION 3.9)
project(xcikit C CXX)

option(XCI_WITH_VULKAN "Build libxci-graphics. Requires Vulkan and GLFW." ON)
option(XCI_WITH_TINFO "Link with TInfo (from NCurses) and use it for TTY control sequences." OFF)
option(XCI_WITH_ZIP "Link xci-core with libzip and use it for ZIP format in VFS." OFF)

set(XCI_SHARE_DIR "${CMAKE_CURRENT_LIST_DIR}/share" CACHE PATH "Location of share dir (runtime data)")
option(XCI_DEBUG_VULKAN "Log info about Vulkan calls and errors." OFF)
option(XCI_EMBED_SHADERS "Embed shader files into binaries to avoid dependency on share dir." OFF)
option(XCI_DEBUG_TRACE "Enable trace log messages." OFF)
option(XCI_DEBUG_MARKUP_DUMP_TOKENS "Text markup parser debugging." OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(XciBuildOptions)

if (EXISTS ${CMAKE_BINARY_DIR}/conan_paths.cmake)
    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
endif()


# ------------ #
# Dependencies #
# ------------ #

if (XCI_WITH_VULKAN)
    # RectangleBinPack (xci-text)
    add_subdirectory(ext/rbp)
endif()

# PEGTL
find_package(pegtl REQUIRED)

# range-v3
find_package(range-v3 REQUIRED)

# Catch
if (BUILD_TESTS)
    find_package(Catch2)
endif()

# Google Benchmark
if (BUILD_BENCHMARKS)
    find_package(benchmark)
endif ()

# Doxygen
find_package(Doxygen)

# TInfo (ncurses)
if (XCI_WITH_TINFO)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(TINFO REQUIRED tinfo)
endif ()

if (WIN32)
    add_subdirectory(ext/mman-win32)
endif()

# Run configuration checks
include(XciBuildChecks)

# Generate config.h
configure_file(config.h.in include/xci/config.h)


# ------------ #
# Assets       #
# ------------ #

add_subdirectory(share)


# ------------ #
# Libraries    #
# ------------ #

set(XCI_INCLUDE_DIRS
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>)

# Setup RPATH
if (APPLE)
    set(CMAKE_INSTALL_RPATH "@executable_path/../lib")
else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()

add_subdirectory(src/xci/core)
add_subdirectory(src/xci/data)
add_subdirectory(src/xci/script)

if (XCI_WITH_VULKAN)
    add_subdirectory(src/xci/graphics)
    add_subdirectory(src/xci/text)
    add_subdirectory(src/xci/widgets)
endif()


# ------------ #
# Examples     #
# ------------ #

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ------------ #
# Tests        #
# ------------ #

if (Catch2_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif ()


# ------------ #
# Benchmarks   #
# ------------ #

if (benchmark_FOUND)
    add_subdirectory(benchmarks)
endif ()


# ------------- #
# Documentation #
# ------------- #

# Only with CMake v3.9+ (older versions don't have doxygen_add_docs())
if (DOXYGEN_FOUND AND NOT CMAKE_VERSION VERSION_LESS "3.9")
    set(DOXYGEN_OUTPUT_DIRECTORY doc)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
    set(DOXYGEN_SOURCE_BROWSER YES)
    set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
    set(DOXYGEN_SORT_MEMBER_DOCS NO)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
    doxygen_add_docs(doc
        src/xci
        docs
        README.md
        COMMENT "Generate documentation")
endif ()


# ------------- #
# Installation  #
# ------------- #

# Headers
install(DIRECTORY src/xci DESTINATION include
    FILES_MATCHING
    REGEX "xci/(graphics)/[a-z]+$" EXCLUDE  # internal headers
    PATTERN "*.h")

# config.h
install(DIRECTORY ${CMAKE_BINARY_DIR}/include/xci DESTINATION include)

install(EXPORT xcikit
    NAMESPACE xcikit::
    DESTINATION lib/cmake/xcikit
    FILE "xcikitTargets.cmake")

set(XCI_INCLUDE_DIRS include)
include(CMakePackageConfigHelpers)
configure_package_config_file(xcikitConfig.cmake.in xcikitConfig.cmake
    INSTALL_DESTINATION lib/cmake/xcikit
    PATH_VARS XCI_INCLUDE_DIRS)
write_basic_package_version_file(xcikitConfigVersion.cmake
    VERSION 0.1.0
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/xcikitConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/xcikitConfigVersion.cmake
    DESTINATION lib/cmake/xcikit)

# Make our CMake modules available to clients
install(FILES cmake/XciBuildOptions.cmake DESTINATION lib/cmake/xcikit)
