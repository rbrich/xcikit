cmake_minimum_required(VERSION 3.9)
project(xcikit CXX)

# optional components (only xci-core is required)
include(CMakeDependentOption)
option(XCI_DATA "Enable component: xci-data" ON)
option(XCI_SCRIPT "Enable component: xci-script" ON)
option(XCI_GRAPHICS "Enable component: xci-graphics (requires Vulkan and GLFW)" ON)
CMAKE_DEPENDENT_OPTION(XCI_TEXT "Enable component: xci-text" ON
    "XCI_GRAPHICS" OFF)
CMAKE_DEPENDENT_OPTION(XCI_WIDGETS "Enable component: xci-widgets" ON
    "XCI_GRAPHICS;XCI_TEXT" OFF)

option(XCI_BUILD_TESTS "Disable to skip building tests." ON)
option(XCI_BUILD_EXAMPLES "Disable to skip building examples." ON)
option(XCI_BUILD_BENCHMARKS "Disable to skip building benchmarks." ON)

option(XCI_WITH_TINFO "Link with TInfo (from NCurses) and use it for TTY control sequences." OFF)
option(XCI_WITH_ZIP "Link xci-core with libzip and use it for ZIP format in VFS." OFF)

option(XCI_INSTALL_SHARE_DIR "Install runtime data as a directory (share/xcikit)" OFF)
option(XCI_INSTALL_SHARE_DAR "Install runtime data as DAR archive (share.dar)" ON)
option(XCI_INSTALL_SHARE_ZIP "Install runtime data as ZIP archive (share.zip)" OFF)
option(XCI_EMBED_SHADERS "Embed shader files into binaries to avoid dependency on share dir." OFF)

option(XCI_DEBUG_VULKAN "Log info about Vulkan calls and errors." OFF)
option(XCI_DEBUG_TRACE "Enable trace log messages." OFF)
option(XCI_DEBUG_MARKUP_DUMP_TOKENS "Text markup parser debugging." OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(XciBuildOptions)

if (EXISTS ${CMAKE_BINARY_DIR}/conan_paths.cmake)
    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
endif()


# ------------ #
# Dependencies #
# ------------ #

if (XCI_TEXT)
    # RectangleBinPack (xci-text)
    add_subdirectory(ext/rbp)
endif()

# PEGTL
find_package(pegtl REQUIRED)

# range-v3
find_package(range-v3 REQUIRED)

# Catch
if (XCI_BUILD_TESTS)
    find_package(Catch2)
endif()

# Google Benchmark
if (XCI_BUILD_BENCHMARKS)
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


# ------------ #
# Assets       #
# ------------ #

add_subdirectory(share)

# Location of share dir / archives
set(XCI_SHARE_DIR "share/xcikit")
set(XCI_SHARE_DAR "share/xcikit/share.dar")
set(XCI_SHARE_ZIP "share/xcikit/share.zip")

set(XCI_SHARE)
if (XCI_INSTALL_SHARE_DIR)
    set(XCI_SHARE "${XCI_SHARE_DIR}")
elseif (XCI_INSTALL_SHARE_DAR)
    set(XCI_SHARE "${XCI_SHARE_DAR}")
elseif (XCI_INSTALL_SHARE_ZIP)
    set(XCI_SHARE "${XCI_SHARE_ZIP}")
endif()

# Generate config.h
configure_file(config.h.in include/xci/config.h)


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

if (XCI_DATA)
    add_subdirectory(src/xci/data)
endif()

if (XCI_SCRIPT)
    add_subdirectory(src/xci/script)
endif()

if (XCI_GRAPHICS)
    add_subdirectory(src/xci/graphics)
endif()

if (XCI_TEXT)
    add_subdirectory(src/xci/text)
endif()

if (XCI_WIDGETS)
    add_subdirectory(src/xci/widgets)
endif()


# ------------ #
# Examples     #
# ------------ #

if (XCI_BUILD_EXAMPLES)
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
