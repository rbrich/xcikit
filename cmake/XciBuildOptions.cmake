option(BUILD_SHARED_LIBS "Build shared libs instead of static libs." OFF)
option(BUILD_TESTS "Disable to skip building tests." ON)
option(BUILD_EXAMPLES "Disable to skip building examples." ON)
option(BUILD_BENCHMARKS "Disable to skip building benchmarks." ON)
option(BUILD_FRAMEWORKS "Build shared libs as OSX frameworks. Implies BUILD_SHARED_LIBS." OFF)
option(BUILD_LTO "Enable link-time, whole-program optimizations." OFF)
option(BUILD_WITH_CCACHE "Use ccache as compiler launcher, when available." ON)
option(BUILD_WITH_IWYU "Run iwyu (Include What You Use) on each compiled file, when available." OFF)
option(BUILD_WARNINGS "Enable compiler warnings: -Wall -Wextra ..." ON)
option(BUILD_PEDANTIC "Build with -Wpedantic -Werror." OFF)
option(BUILD_WITH_ASAN "Build with AddressSanitizer." OFF)
option(BUILD_WITH_UBSAN "Build with UndefinedBehaviorSanitizer." OFF)
option(BUILD_WITH_TSAN "Build with ThreadSanitizer." OFF)

# Build type options
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo" "")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if (BUILD_FRAMEWORKS)
    set(BUILD_SHARED_LIBS ON)
endif()

if (BUILD_LTO)
    # check for LTO (fail when unavailable) and enable it globally
    include(CheckIPOSupported)
    check_ipo_supported()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    # additional whole program optimizations only for programs
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fwhole-program-vtables")
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fwhole-program")
    endif()
endif()

if (BUILD_WITH_CCACHE)
    find_program(CCACHE_EXECUTABLE ccache)
    if (CCACHE_EXECUTABLE)
        message(STATUS "Found ccache: ${CCACHE_EXECUTABLE}")
        set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
    endif()
endif()

if (BUILD_WITH_IWYU)
    find_program(IWYU_EXECUTABLE iwyu)
    message(STATUS "Found iwyu: ${IWYU_EXECUTABLE}")
    if (IWYU_EXECUTABLE)
        set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_EXECUTABLE})
    endif()
endif()

if (BUILD_WARNINGS)
    if (MSVC)
        # see: https://gitlab.kitware.com/cmake/cmake/issues/19084
        string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        string(REGEX REPLACE "/W[0-4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
        # Suppressed warnings:
        # - C4100: unreferenced formal parameter (noisy)
        # - C4200: nonstandard extension used: zero-sized array in struct/union (intentional)
        # - C4244: conversion from 'int' to 'uint8_t' ... (FP)
        add_compile_options(/W4 /wd4100 /wd4200 /wd4244)
    else()
        add_compile_options(-Wall -Wextra
            -Wno-unused-parameter
            -Wno-missing-field-initializers)
    endif()
endif()

if (BUILD_PEDANTIC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic -Werror")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
endif()

if (BUILD_WITH_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif ()

if (BUILD_WITH_UBSAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
endif ()

if (BUILD_WITH_TSAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
endif ()

# Strip dead-code
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-dead_strip")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-dead_strip")
endif ()

# To get useful debuginfo, we need frame pointer
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer")
