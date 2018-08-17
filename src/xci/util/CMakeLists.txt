set(XCI_UTIL_SOURCES
    file.cpp
    file.h
    FileWatch.cpp
    FileWatch.h
    format.cpp
    format.h
    FpsCounter.cpp
    FpsCounter.h
    geometry.h
    log.cpp
    log.h
    SharedLibrary.cpp
    SharedLibrary.h
    string.cpp
    string.h
    rtti.cc
    rtti.h
    Term.cpp
    Term.h
    sys.cpp
    sys.h)


# Choose FileWatch implementation
if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    list(APPEND XCI_UTIL_SOURCES
        filewatch/FileWatchInotify.cpp
        filewatch/FileWatchInotify.h)
    set(FILEWATCH_IMPL "XCI_FILEWATCH_INOTIFY")
elseif (CMAKE_SYSTEM_NAME STREQUAL Darwin)
    list(APPEND XCI_UTIL_SOURCES
        filewatch/FileWatchKqueue.cpp
        filewatch/FileWatchKqueue.h)
    set(FILEWATCH_IMPL "XCI_FILEWATCH_KQUEUE")
else()
    list(APPEND XCI_UTIL_SOURCES
        filewatch/FileWatchDummy.cpp
        filewatch/FileWatchDummy.h)
    set(FILEWATCH_IMPL "XCI_FILEWATCH_DUMMY")
endif()


add_library(${XCI_UTIL_LIBRARY} ${XCI_UTIL_SOURCES})
target_link_libraries(${XCI_UTIL_LIBRARY}
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ${XCI_COMPAT_LIBRARY})
target_include_directories(${XCI_UTIL_LIBRARY} PUBLIC ${XCI_INCLUDE_DIRS})
target_include_directories(${XCI_UTIL_LIBRARY} PRIVATE ${PEGTL_INCLUDE_DIRS})
target_compile_definitions(${XCI_UTIL_LIBRARY} PRIVATE ${FILEWATCH_IMPL})

# Link with tinfo if available
if (XCI_WITH_TINFO)
    target_link_libraries(${XCI_UTIL_LIBRARY} ${TINFO_LIBRARIES})
    target_include_directories(${XCI_UTIL_LIBRARY} PRIVATE ${TINFO_INCLUDE_DIRS})
    target_compile_options(${XCI_UTIL_LIBRARY} PRIVATE ${TINFO_CFLAGS})
endif()

install(TARGETS ${XCI_UTIL_LIBRARY}
    EXPORT xcikit
    DESTINATION lib
    INCLUDES DESTINATION include)
