option(BUILD_SHARED_LIBS "Build shared libs instead of static libs." OFF)
option(BUILD_WITH_CCACHE "Use ccache as compiler launcher, when available." ON)
option(BUILD_WITH_IWYU "Run iwyu (Include What You Use) on each compiled file, when available." OFF)
option(BUILD_PEDANTIC "Build with -Wpedantic -Werror." OFF)
option(BUILD_WITH_ASAN "Build with AddressSanitizer." OFF)
option(BUILD_WITH_UBSAN "Build with UndefinedBehaviorSanitizer." OFF)

if (BUILD_WITH_CCACHE)
    find_program(CCACHE_EXECUTABLE ccache)
    message(STATUS "Found ccache: ${CCACHE_EXECUTABLE}")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
endif()

if (BUILD_WITH_IWYU)
    find_program(IWYU_EXECUTABLE iwyu)
    message(STATUS "Found iwyu: ${IWYU_EXECUTABLE}")
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_EXECUTABLE})
endif()

if (BUILD_PEDANTIC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic -Werror")
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-vla -Wno-gnu-zero-variadic-macro-arguments")
    endif()
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-vla-extension -Wno-gnu-zero-variadic-macro-arguments")
    endif()
endif()

if (BUILD_WITH_ASAN)
    add_compile_options(-fsanitize=address)
endif ()

if (BUILD_WITH_UBSAN)
    add_compile_options(-fsanitize=undefined)
endif ()

# Strip dead-code
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-dead_strip")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-dead_strip")
endif ()

# Strip dead-code
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-dead_strip")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-dead_strip")
endif ()
