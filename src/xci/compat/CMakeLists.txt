# ------------------- #
# Library: xci-compat #
# ------------------- #

add_library(xci-compat INTERFACE)
add_library(xcikit::xci-compat ALIAS xci-compat)

target_sources(xci-compat
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_SOURCE_DIR}/src
    FILES
        dl.h
        endian.h
        float128.h
        int128.h
        macros.h
        unistd.h
        windows.h
)

# ------- #
# Install #
# ------- #

if (XCI_INSTALL_DEVEL)
    install(TARGETS xci-compat
        EXPORT xcikit
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()
