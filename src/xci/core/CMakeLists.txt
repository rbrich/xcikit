# ----------------- #
# Library: xci-core #
# ----------------- #

add_library(xci-core
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
    $<TARGET_OBJECTS:third_party::widechar_width>
    )
add_library(xcikit::xci-core ALIAS xci-core)

# PUBLIC to set CXX standard also on dependees (i.e. all other xci libs + any program using them)
target_compile_features(xci-core PUBLIC cxx_std_${XCI_CXX_STANDARD})

target_precompile_headers(xci-core PRIVATE <fmt/format.h>)

if (NOT WIN32)
    target_sources(xci-core PRIVATE
        FileTree.cpp
        )
endif()

# Choose EventLoop implementation
if (APPLE)
    target_sources(xci-core PRIVATE
        event/kqueue/EventLoop.cpp
        event/kqueue/IOWatch.cpp
        event/kqueue/EventWatch.cpp
        event/kqueue/FSWatch.cpp
        event/kqueue/SignalWatch.cpp
        event/kqueue/TimerWatch.cpp
        )
elseif (WIN32)
    target_sources(xci-core PRIVATE
        event/iocp/EventLoop.cpp
        event/iocp/IOWatch.cpp
        event/iocp/EventWatch.cpp
        event/iocp/FSWatch.cpp
        event/iocp/SignalWatch.cpp
        event/iocp/TimerWatch.cpp
        )
elseif (NOT EMSCRIPTEN)
    target_sources(xci-core PRIVATE
        event/epoll/EventLoop.cpp
        event/epoll/EventWatch.cpp
        event/epoll/FSWatch.cpp
        event/epoll/IOWatch.cpp
        event/epoll/SignalWatch.cpp
        event/epoll/TimerWatch.cpp
        )
endif()
if (NOT EMSCRIPTEN)
    target_sources(xci-core PRIVATE dispatch.cpp)
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
    )
target_include_directories(xci-core
    PUBLIC
        ${XCI_INCLUDE_DIRS}
    PRIVATE
        $<TARGET_PROPERTY:third_party::widechar_width,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:taocpp::pegtl,INTERFACE_INCLUDE_DIRECTORIES>
    )

# Link with tinfo if available
if (XCI_WITH_TERMINFO)
    target_link_libraries(xci-core PRIVATE Terminfo::terminfo)
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
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
