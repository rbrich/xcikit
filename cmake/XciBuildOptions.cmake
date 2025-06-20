# basic build options
option(BUILD_SHARED_LIBS "Build shared libs instead of static libs." OFF)
option(BUILD_FRAMEWORKS "Build shared libs as OSX frameworks. Implies BUILD_SHARED_LIBS." OFF)
option(BUILD_WITH_DEBUG_INFO "Generate debug info also for Release, MinSizeRel. Doesn't affect Debug, RelWithDebInfo." OFF)

# sanitizers (runtime checking tools)
option(BUILD_WITH_ASAN "Build with AddressSanitizer." OFF)
option(BUILD_WITH_LSAN "Build with standalone LeakSanitizer." OFF)
option(BUILD_WITH_UBSAN "Build with UndefinedBehaviorSanitizer." OFF)
option(BUILD_WITH_TSAN "Build with ThreadSanitizer." OFF)
option(BUILD_HARDENING "Build with std lib hardening." OFF)

# warnings (+ compile time checking tools)
option(ENABLE_WARNINGS "Enable compiler warnings: -Wall -Wextra ..." ON)
option(FAIL_ON_WARNINGS "Treat warnings as errors. Applies also to clang-tidy." OFF)
option(ENABLE_TIDY "Run clang-tidy on each compiled file, when available." OFF)
option(ENABLE_IWYU "Run iwyu (Include What You Use) on each compiled file, when available." OFF)

# optimizations
if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    set(MINSIZE ON)
else()
    set(MINSIZE OFF)
endif()
option(ENABLE_LTO "Enable link-time, whole-program optimizations." ${MINSIZE})
option(ENABLE_OZ "Only MinSizeRel: replace default -Os flag by -Oz." OFF)
option(ENABLE_CCACHE "Use ccache as compiler launcher, when available." ON)
set(USE_LD "" CACHE STRING "Use alternative linker command (-fuse-ld).")

# cosmetics
option(FORCE_COLORS "Force colored compiler output." OFF)


set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)  # override to "default" for shared libs

if (BUILD_FRAMEWORKS)
    set(BUILD_SHARED_LIBS ON)
endif()

if (BUILD_WITH_DEBUG_INFO)
    if (NOT MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
        set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -g")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Z7")
        set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /Z7")
    endif()
endif()

if (USE_LD)
    add_link_options("-fuse-ld=${USE_LD}")
endif()

if (ENABLE_LTO)
    if (EMSCRIPTEN)
        # Emscripten supports LTO. The check may fail though, so skip it.
        add_compile_options(-flto)
        add_link_options(-flto)
    else()
        # check for LTO (fail when unavailable) and enable it globally
        include(CheckIPOSupported)
        check_ipo_supported()
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
    # additional whole program optimizations only for programs
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fwhole-program-vtables")
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fwhole-program")
    endif()
endif()

if (ENABLE_CCACHE)
    find_program(CCACHE_EXECUTABLE NAMES sccache ccache)
    if (CCACHE_EXECUTABLE)
        message(STATUS "Found ccache: ${CCACHE_EXECUTABLE}")
        set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
    endif()
endif()

if (ENABLE_TIDY)
    find_program(CLANG_TIDY_EXECUTABLE NAMES "clang-tidy")
    if (CLANG_TIDY_EXECUTABLE)
        message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXECUTABLE}")
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}")
        if (FORCE_COLORS)
            set(CMAKE_CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}" --use-color)
        endif()
        if (FAIL_ON_WARNINGS)
            set(CMAKE_CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}" --warnings-as-errors=*)
        endif()
    else()
        message(FATAL_ERROR "Did not find clang-tidy!")
    endif()
endif()

if (ENABLE_IWYU)
    find_program(IWYU_EXECUTABLE include-what-you-use iwyu)
    message(STATUS "Found iwyu: ${IWYU_EXECUTABLE}")
    if (IWYU_EXECUTABLE)
        set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_EXECUTABLE})
    endif()
endif()

if (ENABLE_WARNINGS)
    if (MSVC)
        # see: https://gitlab.kitware.com/cmake/cmake/issues/19084
        string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        string(REGEX REPLACE "/W[0-4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
        # Suppressed warnings:
        # - C4100: unreferenced formal parameter (noisy)
        # - C4146: unary minus operator applied to unsigned type, result still unsigned (intentional)
        # - C4200: nonstandard extension used: zero-sized array in struct/union (intentional)
        # - C4201: nonstandard extension used: nameless struct/union (intentional)
        # - C4244: conversion from 'int' to 'uint8_t' ... (FP)
        # - C4267: warns on static_cast<uint32_t>(size_t_value) - noisy and irrelevant
        # - C4324: structure was padded due to alignment specifier (FP? triggered by std::variant)
        # - C4702: unreachable code (tons of FPs in std::visit)
        # - C5105: macro expansion producing 'defined' has undefined behavior (only in Windows headers)
        add_compile_options(/W4 /wd4100 /wd4146 /wd4200 /wd4201 /wd4244 /wd4267 /wd4324 /wd4702 /wd5105)
        # https://docs.microsoft.com/en-us/cpp/c-runtime-library/compatibility?view=vs-2019
        add_compile_definitions(
            _CRT_DECLARE_NONSTDC_NAMES=0
            _CRT_SECURE_NO_WARNINGS
            _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1
            _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING
            _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
    else()
        add_compile_options(-Wall -Wextra -Wundef -Werror=switch
            -Wpointer-arith
            -Wextra-semi
            -Wno-unused-parameter
            -Wno-missing-field-initializers)
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # Suppress (subjectively wrong):
            # warning: suggest braces around initialization of subobject [-Wmissing-braces]
            add_compile_options(-Wno-missing-braces)
        endif()
        if (FAIL_ON_WARNINGS)
            add_compile_options(-Werror)
        endif()
    endif()
endif()

if (BUILD_WITH_ASAN)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif ()

if (BUILD_WITH_LSAN)
    add_compile_options(-fsanitize=leak)
    add_link_options(-fsanitize=leak)
endif ()

if (BUILD_WITH_UBSAN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif ()

if (BUILD_WITH_TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif ()

if (BUILD_HARDENING)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        # https://gcc.gnu.org/onlinedocs/libstdc++/manual/debug_mode_using.html
        add_compile_definitions(-D_GLIBCXX_DEBUG)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # https://libcxx.llvm.org/Hardening.html
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            add_compile_definitions(-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG)
        else ()
            add_compile_definitions(-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE)
        endif ()
    endif ()
endif ()

# Strip dead-code
if (APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-dead_strip")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-dead_strip")
endif ()

# To get useful debuginfo, we need frame pointer
if (NOT MSVC)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer")
    if (BUILD_WITH_DEBUG_INFO)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-omit-frame-pointer")
    endif()
endif()

# MinSizeRel - enable more optimization (-Oz)
if (ENABLE_OZ)
    string(REGEX REPLACE "-Os" "-Oz" CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
endif()

if (MSVC)
    # Enable standards-conforming compiler behavior
    add_compile_options(/permissive- /Zc:preprocessor /Zc:inline /Zc:__cplusplus)
    # Read all source files as utf-8
    add_compile_options(/utf-8)
    # xci/script/Machine.cpp : fatal error C1128: number of sections exceeded object file format limit: compile with /bigobj
    add_compile_options(/bigobj)
endif()

if (EMSCRIPTEN)
    add_compile_options(-sDISABLE_EXCEPTION_CATCHING=0)
    add_link_options(-sDISABLE_EXCEPTION_CATCHING=0)
endif()

if (FORCE_COLORS)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options (-fdiagnostics-color=always)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options (-fcolor-diagnostics)
    endif ()
endif()
