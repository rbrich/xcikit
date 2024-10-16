# ------------ #
# Dependencies #
# ------------ #

find_package(Freetype REQUIRED)
if (NOT TARGET Freetype::Freetype AND TARGET freetype)
    # vcpkg is missing the standard target for some reason
    add_library(Freetype::Freetype ALIAS freetype)
endif()

find_package(harfbuzz REQUIRED)


# ------------ #
# Libraries    #
# ------------ #

add_library(xci-text
    FontFace.cpp
    FontLibrary.cpp
    FontTexture.cpp
    Font.cpp
    GlyphCluster.cpp
    Layout.cpp
    Markup.cpp
    Style.cpp
    Text.cpp
    layout/Page.cpp
    $<TARGET_OBJECTS:third_party::rbp>
    )
add_library(xcikit::xci-text ALIAS xci-text)

target_precompile_headers(xci-text REUSE_FROM xci-core)

target_include_directories(xci-text
    PUBLIC
        ${XCI_INCLUDE_DIRS}
    )

target_link_libraries(xci-text
    PUBLIC
        xci-core
        xci-vfs
        xci-graphics
    PRIVATE
        Freetype::Freetype
        harfbuzz::harfbuzz
        $<COMPILE_ONLY:third_party::rbp>
        $<COMPILE_ONLY:taocpp::pegtl>
    )

if (BUILD_SHARED_LIBS)
    set_target_properties(xci-text PROPERTIES CXX_VISIBILITY_PRESET default)
endif()

if (APPLE)
    set_target_properties(xci-text PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup")
endif()

if (BUILD_FRAMEWORKS)
    set_target_properties(xci-text PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER cz.xci.xci-text
        # "current version" in semantic format in Mach-O binary file
        VERSION 1.0.0
        # "compatibility version" in semantic format in Mach-O binary file
        SOVERSION 1.0.0
        PUBLIC_HEADER xci-text.h
        )
endif()

if (BUILD_SHARED_LIBS OR XCI_INSTALL_DEVEL)
    install(TARGETS xci-text EXPORT xcikit DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
