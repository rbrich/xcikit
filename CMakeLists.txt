# 3.18.4 in Debian 11
# 3.16.3 in Emscripten Docker image
# 3.16 introduced CMAKE_MESSAGE_INDENT
cmake_minimum_required(VERSION 3.16)
file(STRINGS "VERSION" XCI_VERSION)
project(xcikit
    VERSION "${XCI_VERSION}"
    LANGUAGES CXX)

# Optional libraries (only xci-core is required)
include(CMakeDependentOption)
option(XCI_DATA "Enable component: xci-data" ON)
option(XCI_SCRIPT "Enable component: xci-script" ON)
option(XCI_GRAPHICS "Enable component: xci-graphics (requires Vulkan and GLFW)" ON)
cmake_dependent_option(XCI_TEXT "Enable component: xci-text" ON
    "XCI_GRAPHICS" OFF)
cmake_dependent_option(XCI_WIDGETS "Enable component: xci-widgets" ON
    "XCI_GRAPHICS;XCI_TEXT" OFF)

# Libraries are always built. The rest is optional:
option(XCI_BUILD_TOOLS "Disable to skip building tools." ON)
option(XCI_BUILD_EXAMPLES "Disable to skip building examples." ON)
option(XCI_BUILD_TESTS "Disable to skip building tests." ON)
option(XCI_BUILD_BENCHMARKS "Disable to skip building benchmarks." ON)

# Optional installation artifacts:
option(XCI_INSTALL_DEVEL "Install headers, CMake config, static libs." ON)

# Dependencies
option(XCI_WITH_TERMINFO "Link with TermInfo and use it for TTY control sequences." OFF)
option(XCI_WITH_ZIP "Link xci-core with libzip and use it for ZIP format in VFS." OFF)
option(XCI_WITH_HYPERSCAN "Hyperscan library is required to build ff tool." OFF)

# negate it for use as option() initial value
if (EMSCRIPTEN)
    set(NOT_EMSCRIPTEN OFF)
else()
    set(NOT_EMSCRIPTEN ON)
endif()

option(XCI_INSTALL_SHARE_DIR "Install runtime data as a directory (share/xcikit)" ${NOT_EMSCRIPTEN})
option(XCI_INSTALL_SHARE_DAR "Install runtime data as DAR archive (share.dar)" OFF)
option(XCI_INSTALL_SHARE_ZIP "Install runtime data as ZIP archive (share.zip)" OFF)

option(XCI_LISTDIR_GETDENTS "Use getdents syscall instead of readdir for tools/find_file." ${NOT_EMSCRIPTEN})

option(XCI_DEBUG_VULKAN "Log info about Vulkan calls and errors." OFF)
option(XCI_DEBUG_TRACE "Enable trace log messages." OFF)
option(XCI_DEBUG_MARKUP_DUMP_TOKENS "Text markup parser debugging." OFF)

option(XCI_PCH_CATCH2 "Precompile <catch2/catch.hpp> header." OFF)

# The C++20 standard will stay as minimal requirement until C++23 is well established among compilers.
# That means at least until year 2024. By that time, xcikit should have an actual release branch.
# Another major version will be created before bumping the requirement.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(XCIKIT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
if (CMAKE_CROSSCOMPILING)
    # Needed for Emscripten to find packages in build dir
    list(APPEND CMAKE_FIND_ROOT_PATH "/")
endif()
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

include(XciBuildOptions)
include(ConanInit)


# ------------ #
# Dependencies #
# ------------ #

add_subdirectory(third_party)

# Threads (always from system)
find_package(Threads REQUIRED)

# PEGTL
find_package(pegtl 3.1.0 REQUIRED)

# range-v3
find_package(range-v3 REQUIRED)

# fmtlib
find_package(fmt REQUIRED)

# magic enum
find_package(magic_enum REQUIRED)

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
if (XCI_WITH_TERMINFO)
    find_library(TERMINFO_LIBRARY NAMES terminfo tinfo curses ncurses ncursesw)
    if (TERMINFO_LIBRARY)
        message(STATUS "Found terminfo: ${TERMINFO_LIBRARY}")
        add_library(terminfo::terminfo INTERFACE IMPORTED)
        set_target_properties(terminfo::terminfo PROPERTIES
            INTERFACE_LINK_LIBRARIES "${TERMINFO_LIBRARY}")
    endif()
endif ()

# Run configuration checks
include(XciBuildChecks)


# ------------ #
# Assets       #
# ------------ #

add_subdirectory(share)

# Location of share dir / archives
if (EMSCRIPTEN)
    # Directory inside Emscripten VFS
    set(XCI_SHARE_DIR "share")
    set(XCI_SHARE_DAR "share.dar")
    set(XCI_SHARE_ZIP "share.zip")
else()
    set(XCI_SHARE_DIR "share/xcikit")
    set(XCI_SHARE_DAR "share/xcikit/share.dar")
    set(XCI_SHARE_ZIP "share/xcikit/share.zip")
endif()

set(XCI_SHARE)
if (XCI_INSTALL_SHARE_DIR OR EMSCRIPTEN)
    set(XCI_SHARE "${XCI_SHARE_DIR}")
elseif (XCI_INSTALL_SHARE_DAR)
    set(XCI_SHARE "${XCI_SHARE_DAR}")
elseif (XCI_INSTALL_SHARE_ZIP)
    set(XCI_SHARE "${XCI_SHARE_ZIP}")
endif()


# ------------ #
# config.h     #
# ------------ #

# Generate config.h
configure_file(config.h.in include/xci/config.h)


# ------------ #
# Libraries    #
# ------------ #

set(XCI_INCLUDE_DIRS
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
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
# Tools     #
# ------------ #

if (XCI_BUILD_TOOLS)
    if (XCI_WITH_HYPERSCAN)
        add_subdirectory(tools/find_file)
    endif()
    if (XCI_SCRIPT)
        add_subdirectory(tools/fire_script)
    endif()
endif()


# ------------ #
# Tests        #
# ------------ #

if (Catch2_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif()


# ------------ #
# Benchmarks   #
# ------------ #

if (benchmark_FOUND)
    add_subdirectory(benchmarks)
endif()


# ------------- #
# Documentation #
# ------------- #

if (DOXYGEN_FOUND)
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
endif()


# ------------- #
# Summary       #
# ------------- #

function(print_flag msg flag)
    if ($ENV{XCI_CMAKE_COLORS})
        string (ASCII 27 esc)
        if (flag)
            string(REPLACE "?" "${esc}[1\;32m[x]" msg1 ${msg})
        else()
            string(REPLACE "?" "${esc}[90m[ ]" msg1 ${msg})
        endif()
        string(REPLACE "!" "${esc}[0m " msg ${msg1})
    else()
        if (flag)
            string(REPLACE "?" "[x]" msg1 ${msg})
        else()
            string(REPLACE "?" "[ ]" msg1 ${msg})
        endif()
        string(REPLACE "!" " " msg ${msg1})
    endif()
    message(${msg})
endfunction()

function(print_trgt msg target)
    if (TARGET ${target})
        print_flag(${msg} ON)
    else()
        print_flag(${msg} OFF)
    endif()
endfunction()

macro(push_indent)
    list(APPEND CMAKE_MESSAGE_INDENT "    ")
endmacro()

macro(pop_indent)
    list(POP_BACK CMAKE_MESSAGE_INDENT)
endmacro()

message("Optional components:")
push_indent()
print_flag("? libraries!                   " ON)
if (TRUE)
    push_indent()
    print_flag("? xci-core!                    " ON)
    print_flag("? xci-data!                    " ${XCI_DATA})
    print_flag("? xci-script!                  " ${XCI_SCRIPT})
    print_flag("? xci-graphics!                " ${XCI_GRAPHICS})
    print_flag("? xci-text!                    " ${XCI_TEXT})
    print_flag("? xci-widgets!                 " ${XCI_WIDGETS})
    pop_indent()
endif()
print_flag("? tools!                       " ${XCI_BUILD_TOOLS})
if (${XCI_BUILD_TOOLS})
    push_indent()
    print_trgt("? ff!   (req. Hyperscan)       " ff)
    print_trgt("? fire! (req. xci-script)      " fire)
    pop_indent()
endif()
print_flag("? examples!                    " ${XCI_BUILD_EXAMPLES})
print_flag("? tests!                       " ${XCI_BUILD_TESTS})
print_flag("? benchmarks!                  " ${XCI_BUILD_BENCHMARKS})
pop_indent()


# ------------- #
# Installation  #
# ------------- #

if (XCI_INSTALL_DEVEL)
    # Headers
    install(DIRECTORY src/xci DESTINATION include
        FILES_MATCHING
        PATTERN "*.h")

    # config.h
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/xci DESTINATION include)

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
        COMPATIBILITY SameMajorVersion)
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/xcikitConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/xcikitConfigVersion.cmake
        DESTINATION lib/cmake/xcikit)

    # Make our CMake modules available to clients
    install(FILES cmake/XciBuildOptions.cmake DESTINATION lib/cmake/xcikit)
endif()


# ------------- #
# Packaging     #
# ------------- #

include(CPack)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Collection of C++ libraries for drawing 2D graphics, rendering text and more.")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/rbrich/xcikit")
set(CPACK_PACKAGE_VENDOR "xci.cz")
set(CPACK_PACKAGE_CONTACT "radek.brich@devl.cz")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.adoc")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_STRIP_FILES "ON")
