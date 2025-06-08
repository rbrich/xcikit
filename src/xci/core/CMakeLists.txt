# ----------------- #
# Library: xci-core #
# ----------------- #

add_library(xci-core)
add_library(xcikit::xci-core ALIAS xci-core)

target_sources(xci-core
    PRIVATE
        ArgParser.cpp
        file.cpp
        EditBuffer.cpp
        EditLine.cpp
        ResourceUsage.cpp
        FpsCounter.cpp
        log.cpp
        SharedLibrary.cpp
        string.cpp
        sys.cpp
        rtti.cpp
        TermCtl.cpp
        container/StringPool.cpp
        parser/raw_string.cpp
        $<TARGET_OBJECTS:third_party::widechar_width>
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_SOURCE_DIR}/src
    FILES
        ArgParser.h
        bit.h
        Buffer.h
        EditBuffer.h
        EditLine.h
        error.h
        event.h
        file.h
        FpsCounter.h
        log.h
        memoization.h
        memory.h
        mixin.h
        ResourceUsage.h
        rtti.h
        SharedLibrary.h
        string.h
        sys.h
        TermCtl.h
        container/ChunkedStack.h
        container/FlatSet.h
        container/IndexedMap.h
        container/StaticVec.h
        container/StringPool.h
        macros/foreach.h
        parser/raw_string.h
        parser/unescape.h
        parser/unescape_rules.h
        template/helpers.h
        template/ToFunctionPtr.h
    )

# PUBLIC to set CXX standard also on dependees (i.e. all other xci libs + any program using them)
target_compile_features(xci-core PUBLIC cxx_std_${XCI_CXX_STANDARD})

target_precompile_headers(xci-core PRIVATE <fmt/format.h>)

if (NOT WIN32)
    target_sources(xci-core
        PRIVATE
            FileTree.cpp
        PUBLIC FILE_SET HEADERS FILES
            FileTree.h
            listdir.h
        )
endif()

# Choose EventLoop implementation
if (APPLE)
    target_sources(xci-core
        PRIVATE
            event/kqueue/EventLoop.cpp
            event/kqueue/IOWatch.cpp
            event/kqueue/EventWatch.cpp
            event/kqueue/FSWatch.cpp
            event/kqueue/SignalWatch.cpp
            event/kqueue/TimerWatch.cpp
        PUBLIC FILE_SET HEADERS FILES
            event/kqueue/EventLoop.h
            event/kqueue/IOWatch.h
            event/kqueue/EventWatch.h
            event/kqueue/FSWatch.h
            event/kqueue/SignalWatch.h
            event/kqueue/TimerWatch.h
        )
elseif (WIN32)
    target_sources(xci-core
        PRIVATE
            event/iocp/EventLoop.cpp
            event/iocp/IOWatch.cpp
            event/iocp/EventWatch.cpp
            event/iocp/FSWatch.cpp
            event/iocp/SignalWatch.cpp
            event/iocp/TimerWatch.cpp
        PUBLIC FILE_SET HEADERS FILES
            event/iocp/EventLoop.h
            event/iocp/IOWatch.h
            event/iocp/EventWatch.h
            event/iocp/FSWatch.h
            event/iocp/SignalWatch.h
            event/iocp/TimerWatch.h
        )
elseif (NOT EMSCRIPTEN)
    target_sources(xci-core
        PRIVATE
            event/epoll/EventLoop.cpp
            event/epoll/EventWatch.cpp
            event/epoll/FSWatch.cpp
            event/epoll/IOWatch.cpp
            event/epoll/SignalWatch.cpp
            event/epoll/TimerWatch.cpp
        PUBLIC FILE_SET HEADERS FILES
            event/epoll/EventLoop.h
            event/epoll/EventWatch.h
            event/epoll/FSWatch.h
            event/epoll/IOWatch.h
            event/epoll/SignalWatch.h
            event/epoll/TimerWatch.h
        )
endif()
if (NOT EMSCRIPTEN)
    target_sources(xci-core
        PRIVATE
            dispatch.cpp
        PUBLIC FILE_SET HEADERS FILES
            dispatch.h
        )
endif()

# Choose Logger::default_handler implementation
if (EMSCRIPTEN)
    target_sources(xci-core PRIVATE log_emscripten.cpp)
else()
    target_sources(xci-core PRIVATE log_termctl.cpp)
endif()

target_link_libraries(xci-core
    PUBLIC
        fmt::fmt
        Threads::Threads
    PRIVATE
        ${CMAKE_DL_LIBS}
        $<COMPILE_ONLY:taocpp::pegtl>
        $<COMPILE_ONLY:third_party::widechar_width>
    )
target_include_directories(xci-core
    PUBLIC
        ${XCI_INCLUDE_DIRS}
    )

# Link with tinfo if available
if (XCI_WITH_TERMINFO)
    target_link_libraries(xci-core PRIVATE Terminfo::terminfo)
endif()

# backtrace
if (NOT EMSCRIPTEN)
    target_sources(xci-core
        PRIVATE
            backtrace.cpp
        PUBLIC FILE_SET HEADERS FILES
            backtrace.h
        )
endif()
if (WIN32)
    target_link_libraries(xci-core PRIVATE dbghelp)
endif()

if (BUILD_SHARED_LIBS)
    set_target_properties(xci-core PROPERTIES CXX_VISIBILITY_PRESET default)
endif()

if (BUILD_FRAMEWORKS)
    set_target_properties(xci-core PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER cz.xci.xci-core
        # "current version" in semantic format in Mach-O binary file
        VERSION 1.0.0
        # "compatibility version" in semantic format in Mach-O binary file
        SOVERSION 1.0.0
        PUBLIC_HEADER xci-core.h
        )
endif()


# ------- #
# Install #
# ------- #

if (BUILD_SHARED_LIBS OR XCI_INSTALL_DEVEL)
    install(TARGETS xci-core
        EXPORT xcikit
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()
