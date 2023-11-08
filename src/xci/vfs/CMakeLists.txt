# ---------------- #
# Library: xci-vfs #
# ---------------- #

add_library(xci-vfs
    Vfs.cpp
    RealDirectory.cpp
    DarArchive.cpp
    WadArchive.cpp
    ZipArchive.cpp
)
add_library(xcikit::xci-vfs ALIAS xci-vfs)
target_precompile_headers(xci-vfs REUSE_FROM xci-core)
target_link_libraries(xci-vfs PUBLIC xci-core)
if (BUILD_SHARED_LIBS)
    set_target_properties(xci-vfs PROPERTIES CXX_VISIBILITY_PRESET default)
endif()

# VFS requires libzip
find_package(libzip REQUIRED)
target_link_libraries(xci-vfs PRIVATE libzip::zip)


# ------- #
# Install #
# ------- #

if (BUILD_SHARED_LIBS OR XCI_INSTALL_DEVEL)
    install(TARGETS xci-vfs
        EXPORT xcikit
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
