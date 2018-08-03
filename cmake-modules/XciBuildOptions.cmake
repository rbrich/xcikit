option(BUILD_SHARED_LIBS "Build shared libs instead of static libs." OFF)
option(BUILD_WITH_CCACHE "Use ccache as compiler launcher, when available." ON)
option(BUILD_PEDANTIC "Build with -Wpedantic -Werror." OFF)
option(BUILD_WITH_ASAN "Build with AddressSanitizer." OFF)
option(BUILD_WITH_UBSAN "Build with UndefinedBehaviorSanitizer." OFF)

if (BUILD_WITH_CCACHE)
    find_program(CCACHE_EXECUTABLE ccache)
    message(STATUS "Found ccache: ${CCACHE_EXECUTABLE}")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
endif()

if (BUILD_PEDANTIC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic -Wno-vla-extension -Wno-gnu-zero-variadic-macro-arguments -Werror")
endif()

if (BUILD_WITH_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif ()

if (BUILD_WITH_UBSAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
endif ()
