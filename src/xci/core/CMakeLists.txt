# ------------ #
# Libraries    #
# ------------ #

add_library(xci-core
    ArgParser.cpp
    chrono.cpp
    file.cpp
    dispatch.cpp
    format.cpp
    FpsCounter.cpp
    log.cpp
    SharedLibrary.cpp
    string.cpp
    sys.cpp
    rtti.cc
    TermCtl.cpp
    Vfs.cpp
    )

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
elseif(WIN32)
    target_sources(xci-core PRIVATE
        event/iocp/EventLoop.cpp
        event/iocp/IOWatch.cpp
        event/iocp/EventWatch.cpp
        event/iocp/FSWatch.cpp
        event/iocp/SignalWatch.cpp
        event/iocp/TimerWatch.cpp
        )
    target_link_libraries(xci-core PRIVATE mman-win32)
else()
    target_sources(xci-core PRIVATE
        event/epoll/EventLoop.cpp
        event/epoll/EventWatch.cpp
        event/epoll/FSWatch.cpp
        event/epoll/IOWatch.cpp
        event/epoll/SignalWatch.cpp
        event/epoll/TimerWatch.cpp
        )
endif()

target_link_libraries(xci-core
    PUBLIC
        Threads::Threads
    PRIVATE
        ${CMAKE_DL_LIBS})
target_include_directories(xci-core
    PUBLIC
        ${XCI_INCLUDE_DIRS}
    PRIVATE
        $<TARGET_PROPERTY:taocpp::pegtl,INTERFACE_INCLUDE_DIRECTORIES>
    )

# Link with tinfo if available
if (XCI_WITH_TINFO)
    target_link_libraries(xci-core PRIVATE ${TINFO_LIBRARIES})
    target_include_directories(xci-core PRIVATE ${TINFO_INCLUDE_DIRS})
    target_compile_options(xci-core PRIVATE ${TINFO_CFLAGS})
endif()

# ZIP support in VFS requires libzip
if (XCI_WITH_ZIP)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(LibZip REQUIRED IMPORTED_TARGET libzip)
    target_link_libraries(xci-core PRIVATE PkgConfig::LibZip)
    target_compile_definitions(xci-core PRIVATE XCI_WITH_ZIP)
endif ()

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

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(xci-core PRIVATE
        _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
        _CRT_SECURE_NO_WARNINGS)
endif()

install(TARGETS xci-core
    EXPORT xcikit
    DESTINATION lib
    INCLUDES DESTINATION include)
