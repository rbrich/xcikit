# 3.28.3 in Ubuntu 24.04 (noble)
# 3.30.3 in Debian 12 (bookworm-backports)
cmake_minimum_required(VERSION 3.27...3.30)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(XciBuildType)

file(STRINGS "VERSION" XCI_VERSION)
project(xcikit
    LANGUAGES CXX
    VERSION "${XCI_VERSION}"
    HOMEPAGE_URL "https://github.com/rbrich/xcikit")

# Disable C++ modules.
# If not disabled, it breaks build and also CheckCXXSourceCompiles unless
# the compiler scanning tool is installed (which is optional).
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

include(GNUInstallDirs)

# Optional libraries (only xci-core is required)
include(CMakeDependentOption)
option(XCI_CONFIG "Enable component: xci-config" ON)
option(XCI_VFS "Enable component: xci-vfs" ON)
option(XCI_WIDGETS "Enable component: xci-widgets" ON)
option(XCI_SCRIPT "Enable component: xci-script" ON)
option(XCI_SCENE "Enable component: xci-scene" ON)
cmake_dependent_option(XCI_TEXT "Enable component: xci-text" ON
    "NOT XCI_WIDGETS" ON)
cmake_dependent_option(XCI_GRAPHICS "Enable component: xci-graphics (requires Vulkan and SDL)" ON
    "NOT XCI_TEXT" ON)
cmake_dependent_option(XCI_DATA "Enable component: xci-data" ON
    "NOT XCI_SCRIPT" ON)

# Libraries are always built. The rest is optional:
option(BUILD_TOOLS "Disable to skip building tools." ON)
option(BUILD_EXAMPLES "Disable to skip building examples." ON)
option(BUILD_TESTS "Disable to skip building tests." ON)
option(BUILD_BENCHMARKS "Disable to skip building benchmarks." ON)

# Individual tools
option(BUILD_DAR_TOOL "Disable to skip building 'dar' (DAR Archive Extractor) tool." ON)
option(BUILD_DATI_TOOL "Disable to skip building 'dati' (Data Inspect) tool." ON)
option(BUILD_FF_TOOL "Disable to skip building 'ff' (Find File) tool." ON)
option(BUILD_FIRE_TOOL "Disable to skip building 'fire' (Fire Script) tool." ON)
option(BUILD_SHED_TOOL "Disable to skip building 'shed' (Shader Editor) tool." ON)
option(BUILD_TC_TOOL "Disable to skip building 'tc' (Term Ctl) tool." ON)

# Optional installation artifacts:
option(XCI_INSTALL_DEVEL "Install headers, CMake config, static libs." ON)
option(XCI_INSTALL_EXAMPLES "Install headers, CMake config, static libs." ON)
option(XCI_INSTALL_TESTS "Install tests." OFF)
option(XCI_INSTALL_BENCHMARKS "Install benchmarks." ON)

# Dependencies
option(XCI_WITH_TERMINFO "Link with TermInfo and use it for TTY control sequences, instead of builtin sequences." OFF)
option(XCI_WITH_HYPERSCAN "Hyperscan / Vectorscan library is required to build ff tool." OFF)

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

include(XciBuildOptions)

# Run configuration checks
include(XciBuildChecks)

# Minimal requirement is (at least partial) C++20.
set(XCI_CXX_STANDARD 20)
if (HAVE_LIBSTDCXX)
    set(CMAKE_CXX_EXTENSIONS ON)  # for __int128
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
    # Enable C++20 ranges in Clang 15
    add_compile_options(-fexperimental-library)
endif()

set(XCIKIT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

if (CMAKE_CROSSCOMPILING)
    # Needed for Emscripten to find packages in build dir
    list(APPEND CMAKE_FIND_ROOT_PATH "/")
endif()
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)


# ------------ #
# Dependencies #
# ------------ #

add_subdirectory(third_party)

# Threads (always from system)
if (NOT WIN32)
    find_package(Threads REQUIRED)
else()
    # Dummy target - workaround for failing find_package(Threads) on Windows with conan_provider.cmake
    add_library(Threads::Threads INTERFACE IMPORTED)
endif()

if (NOT EMSCRIPTEN AND (XCI_DATA OR XCI_VFS))
    find_package(ZLIB REQUIRED)
endif()

# PEGTL
find_package(pegtl REQUIRED)

# SDL
if (XCI_GRAPHICS OR XCI_WIDGETS)
    find_package(SDL2 REQUIRED)
endif()

# range-v3
if (XCI_SCRIPT OR XCI_GRAPHICS)
    find_package(range-v3 REQUIRED)
endif()

# fmtlib
find_package(fmt REQUIRED)

# magic enum
if (XCI_DATA OR BUILD_EXAMPLES)
    find_package(magic_enum REQUIRED)
    # Workaround for Conan package, which has wrong include path
    # (would require <magic_enum/magic_enum.hpp>, which is incompatible with upstream).
    if (EXISTS ${magic_enum_INCLUDE_DIR}/magic_enum)
        target_include_directories(magic_enum::magic_enum SYSTEM BEFORE INTERFACE ${magic_enum_INCLUDE_DIR}/magic_enum)
    endif()
endif()

# Catch
if (BUILD_TESTS)
    find_package(Catch2 3 REQUIRED)
endif()

# Google Benchmark
if (BUILD_BENCHMARKS)
    find_package(benchmark REQUIRED)
endif()

# Doxygen
find_package(Doxygen)

# TInfo (ncurses)
if (XCI_WITH_TERMINFO)
    find_package(Terminfo)
endif ()


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

if (XCI_CONFIG)
    add_subdirectory(src/xci/config)
endif()

if (XCI_VFS)
    add_subdirectory(src/xci/vfs)
endif()

if (XCI_DATA)
    add_subdirectory(src/xci/data)
endif()

if (XCI_SCRIPT)
    add_subdirectory(src/xci/script)
endif()

if (XCI_SCENE)
    add_subdirectory(src/xci/scene)
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

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()


# ------------ #
# Tools     #
# ------------ #

if (BUILD_TOOLS)
    if (BUILD_TC_TOOL)
        add_subdirectory(tools/term_ctl)
    endif()
    if (BUILD_DAR_TOOL)
        add_subdirectory(tools/data_archive)
    endif()
    if (BUILD_FF_TOOL AND XCI_WITH_HYPERSCAN)
        add_subdirectory(tools/find_file)
    endif()
    if (BUILD_DATI_TOOL AND XCI_DATA)
        add_subdirectory(tools/data_inspect)
    endif()
    if (BUILD_FIRE_TOOL AND XCI_SCRIPT)
        add_subdirectory(tools/fire_script)
    endif()
    if (BUILD_SHED_TOOL AND XCI_WIDGETS)
        add_subdirectory(tools/shader_editor)
    endif()
endif()


# ------------ #
# Tests        #
# ------------ #

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()


# ------------ #
# Benchmarks   #
# ------------ #

if (BUILD_BENCHMARKS)
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
    # https://gitlab.kitware.com/cmake/cmake/-/issues/16183
    if (DEFINED ENV{XCI_CMAKE_COLORS} AND NOT WIN32)
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
    print_flag("? xci-config!                  " ${XCI_CONFIG})
    print_flag("? xci-vfs!                     " ${XCI_VFS})
    print_flag("? xci-data!                    " ${XCI_DATA})
    print_flag("? xci-script!                  " ${XCI_SCRIPT})
    print_flag("? xci-scene!                   " ${XCI_SCENE})
    print_flag("? xci-graphics!                " ${XCI_GRAPHICS})
    print_flag("? xci-text!                    " ${XCI_TEXT})
    print_flag("? xci-widgets!                 " ${XCI_WIDGETS})
    pop_indent()
endif()
print_flag("? tools!                       " ${BUILD_TOOLS})
if (${BUILD_TOOLS})
    push_indent()
    print_trgt("? dar!  (req. xci-vfs)         " dar)
    print_trgt("? dati! (req. xci-data)        " dati)
    print_trgt("? ff!   (req. Hyperscan)       " ff)
    print_trgt("? fire! (req. xci-script)      " fire)
    print_trgt("? shed! (req. xci-widgets)     " shed)
    print_trgt("? tc!                          " tc)
    pop_indent()
endif()
print_flag("? examples!                    " ${BUILD_EXAMPLES})
print_flag("? tests!                       " ${BUILD_TESTS})
print_flag("? benchmarks!                  " ${BUILD_BENCHMARKS})
pop_indent()


# ------------- #
# Installation  #
# ------------- #

if (XCI_INSTALL_DEVEL)
    # Headers
    install(DIRECTORY src/xci DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
        PATTERN "*.h")

    # config.h
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/xci DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(EXPORT xcikit
        NAMESPACE xcikit::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xcikit
        FILE "xcikit-targets.cmake")

    set(XCI_INCLUDE_DIRS include)
    set(XCI_PACKAGE_DIR "")
    include(CMakePackageConfigHelpers)
    configure_package_config_file(xcikit-config.cmake.in xcikit-config.cmake
        INSTALL_DESTINATION lib/cmake/xcikit
        PATH_VARS XCI_PACKAGE_DIR XCI_INCLUDE_DIRS)
    write_basic_package_version_file(xcikit-config-version.cmake
        COMPATIBILITY SameMajorVersion)
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/xcikit-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/xcikit-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xcikit)

    # Make our CMake modules available to clients
    install(FILES
        cmake/XciBuildOptions.cmake
        cmake/FindGLSLC.cmake
        cmake/FindTerminfo.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xcikit)
endif()


# ------------- #
# Packaging     #
# ------------- #

include(CPack)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Collection of C++ libraries for drawing 2D graphics, rendering text and more.")
set(CPACK_PACKAGE_VENDOR "xci.cz")
set(CPACK_PACKAGE_CONTACT "radek.brich@devl.cz")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.adoc")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_STRIP_FILES "ON")
