# ------------ #
# Dependencies #
# ------------ #

find_package(libzip REQUIRED)


# ---------------- #
# Library: xci-vfs #
# ---------------- #

add_library(xci-vfs)
add_library(xcikit::xci-vfs ALIAS xci-vfs)

target_sources(xci-vfs
    PRIVATE
        Vfs.cpp
        RealDirectory.cpp
        DarArchive.cpp
        WadArchive.cpp
        ZipArchive.cpp
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_SOURCE_DIR}/src
    FILES
        Vfs.h
        RealDirectory.h
        DarArchive.h
        WadArchive.h
        ZipArchive.h
)

target_precompile_headers(xci-vfs REUSE_FROM xci-core)
target_link_libraries(xci-vfs PUBLIC xci-core)
if (BUILD_SHARED_LIBS)
    set_target_properties(xci-vfs PROPERTIES CXX_VISIBILITY_PRESET default)
endif()

target_link_libraries(xci-vfs PRIVATE libzip::zip)

if (EMSCRIPTEN)
    target_compile_options(xci-vfs PUBLIC -sUSE_ZLIB=1)
    target_link_options(xci-vfs PUBLIC -sUSE_ZLIB=1)
else()
    target_link_libraries(xci-vfs PRIVATE ZLIB::ZLIB)
endif()


# ------- #
# Install #
# ------- #

if (BUILD_SHARED_LIBS OR XCI_INSTALL_DEVEL)
    install(TARGETS xci-vfs
        EXPORT xcikit
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()
