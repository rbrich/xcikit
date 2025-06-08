# ----------------- #
# Library: xci-math #
# ----------------- #

add_library(xci-math INTERFACE)
add_library(xcikit::xci-math ALIAS xci-math)

target_sources(xci-math
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_SOURCE_DIR}/src
    FILES
        geometry.h
        Mat2.h
        Mat3.h
        Mat4.h
        Rect.h
        transform.h
        Vec2.h
        Vec3.h
        Vec4.h
)

# ------- #
# Install #
# ------- #

if (XCI_INSTALL_DEVEL)
    install(TARGETS xci-math
        EXPORT xcikit
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()
