add_library(xci-core
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
    Term.cpp
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
    target_compile_definitions(xci-core PUBLIC XCI_WITH_KQUEUE)
else()
    target_sources(xci-core PRIVATE
        event/epoll/EventLoop.cpp
        event/epoll/EventWatch.cpp
        event/epoll/FSWatch.cpp
        event/epoll/IOWatch.cpp
        event/epoll/SignalWatch.cpp
        event/epoll/TimerWatch.cpp
        )
    target_compile_definitions(xci-core PUBLIC XCI_WITH_EPOLL)
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
    target_link_libraries(xci-core ${TINFO_LIBRARIES})
    target_include_directories(xci-core PRIVATE ${TINFO_INCLUDE_DIRS})
    target_compile_options(xci-core PRIVATE ${TINFO_CFLAGS})
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

install(TARGETS xci-core
    EXPORT xcikit
    DESTINATION lib
    INCLUDES DESTINATION include)
