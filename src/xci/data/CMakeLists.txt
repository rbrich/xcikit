# ------------ #
# Dependencies #
# ------------ #

find_package(pfr QUIET)
if (NOT pfr_FOUND)
    # vcpkg doesn't provide any cmake config, find the headers directly
    find_path(pfr_INCLUDE_DIR NAMES boost/pfr/config.hpp)
    if (pfr_INCLUDE_DIR)
        add_library(pfr::pfr INTERFACE IMPORTED)
        target_include_directories(pfr::pfr INTERFACE ${pfr_INCLUDE_DIR})
    else ()
        message(FATAL_ERROR "pfr not found!")
    endif()
endif()


# ------------ #
# Libraries    #
# ------------ #

add_library(xci-data)
add_library(xcikit::xci-data ALIAS xci-data)

target_sources(xci-data
    PRIVATE
        BinaryWriter.cpp
        BinaryReader.cpp
        Crc32.cpp
        Dumper.cpp
        Schema.cpp
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_SOURCE_DIR}/src
    FILES
        ArchiveBase.h
        BinaryBase.h
        BinaryReader.h
        BinaryWriter.h
        Crc32.h
        Dumper.h
        Schema.h
        coding/leb128.h
    )

target_link_libraries(xci-data
    PUBLIC
        xci-core
        pfr::pfr
        magic_enum::magic_enum
    )

if (EMSCRIPTEN)
    target_compile_options(xci-data PUBLIC -sUSE_ZLIB=1)
    target_link_options(xci-data PUBLIC -sUSE_ZLIB=1)
else()
    target_link_libraries(xci-data PRIVATE ZLIB::ZLIB)
endif()

target_include_directories(xci-data
    PUBLIC
        ${XCI_INCLUDE_DIRS}
    )

if (BUILD_SHARED_LIBS)
    set_target_properties(xci-data PROPERTIES CXX_VISIBILITY_PRESET default)
endif()

if (BUILD_SHARED_LIBS OR XCI_INSTALL_DEVEL)
    install(TARGETS xci-data
        EXPORT xcikit
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()
