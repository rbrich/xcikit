add_library(xci-core
    chrono.cpp
    file.cpp
    FileWatch.cpp
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

# Choose FileWatch implementation
if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_sources(xci-core PRIVATE filewatch/FileWatchInotify.cpp)
    set(FILEWATCH_IMPL "XCI_FILEWATCH_INOTIFY")
elseif (CMAKE_SYSTEM_NAME STREQUAL Darwin)
    target_sources(xci-core PRIVATE filewatch/FileWatchKqueue.cpp)
    set(FILEWATCH_IMPL "XCI_FILEWATCH_KQUEUE")
else()
    target_sources(xci-core PRIVATE filewatch/FileWatchDummy.cpp)
    set(FILEWATCH_IMPL "XCI_FILEWATCH_DUMMY")
endif()

target_link_libraries(xci-core
    PUBLIC
        Threads::Threads
    PRIVATE
        taocpp::pegtl
        ${CMAKE_DL_LIBS})
target_include_directories(xci-core
    PUBLIC
        ${XCI_INCLUDE_DIRS})
target_compile_definitions(xci-core PRIVATE ${FILEWATCH_IMPL})

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
