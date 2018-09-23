cmake_minimum_required(VERSION 3.7)
cmake_policy(VERSION 3.7)
project(xcikit C CXX)

option(XCI_WITH_OPENGL "Link with GLFW and build libxci-graphics-opengl." ON)
option(XCI_WITH_SFML "Link with SFML and build libxci-graphics-sfml." OFF)
option(XCI_WITH_PANDA "Link with Panda3D and build libxci-graphics-panda." OFF)
option(XCI_WITH_TINFO "Link with TInfo (from NCurses) and use it for TTY control sequences." OFF)
option(XCI_BENCHMARK "Enable benchmarks. Requires Google benchmark to be installed and in CMake path." OFF)

set(XCI_SHARE_DIR "${CMAKE_CURRENT_LIST_DIR}/share" CACHE PATH "Location of share dir (runtime data)")
option(XCI_EMBED_SHADERS "Embed shader files into binaries to avoid dependency on share dir." OFF)
option(XCI_DEBUG_TRACE "Enable trace log messages." OFF)
option(XCI_DEBUG_MARKUP_DUMP_TOKENS "Text markup parser debugging." OFF)
option(XCI_DEBUG_OPENGL "Log info about OpenGL calls and errors." OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)
include(XciBuildOptions)

if (EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(KEEP_RPATHS)
endif()


# ------------ #
# Sources      #
# ------------ #

set(XCI_GRAPHICS_SOURCES
    src/xci/graphics/Color.cpp
    src/xci/graphics/Primitives.cpp
    src/xci/graphics/Renderer.cpp
    src/xci/graphics/Shader.cpp
    src/xci/graphics/Shape.cpp
    src/xci/graphics/Sprites.cpp
    src/xci/graphics/View.cpp
    src/xci/graphics/Window.cpp)

set(XCI_GRAPHICS_OPENGL_SOURCES
    src/xci/graphics/opengl/GlPrimitives.cpp
    src/xci/graphics/opengl/GlRenderer.cpp
    src/xci/graphics/opengl/GlShader.cpp
    src/xci/graphics/opengl/GlTexture.cpp
    src/xci/graphics/opengl/GlWindow.cpp)

set(XCI_GRAPHICS_SFML_SOURCES
    src/xci/graphics/sfml/SfmlTexture.cpp
    src/xci/graphics/sfml/SfmlSprites.cpp
    src/xci/graphics/sfml/SfmlView.cpp
    src/xci/graphics/sfml/SfmlWindow.cpp)

set(XCI_GRAPHICS_PANDA_SOURCES
    src/xci/graphics/panda/PandaTexture.cpp
    src/xci/graphics/panda/PandaSprites.cpp
    src/xci/graphics/panda/PandaView.cpp
    src/xci/graphics/panda/PandaWindow.cpp)

set(XCI_TEXT_SOURCES
    src/xci/text/FontLibrary.cpp
    src/xci/text/FontTexture.cpp
    src/xci/text/Font.cpp
    src/xci/text/Text.cpp
    src/xci/text/Layout.cpp
    src/xci/text/Markup.cpp
    src/xci/text/Style.cpp
    src/xci/text/layout/Page.cpp
    src/xci/text/freetype/FtFontLibrary.cpp
    src/xci/text/freetype/FtFontFace.cpp)

set(XCI_WIDGETS_SOURCES
    src/xci/widgets/Button.cpp
    src/xci/widgets/Checkbox.cpp
    src/xci/widgets/FpsDisplay.cpp
    src/xci/widgets/Icon.cpp
    src/xci/widgets/Theme.cpp
    src/xci/widgets/Widget.cpp
    src/xci/widgets/TextInput.cpp
    src/xci/widgets/Form.cpp
    src/xci/widgets/Label.cpp
    src/xci/widgets/TextTerminal.cpp)


# ------------ #
# Dependencies #
# ------------ #

# pthread
find_package(Threads REQUIRED)

# FreeType
include(FindFreetype)

# RectangleBinPack
add_subdirectory(ext/rbp)
set(RBP_LIBRARY rbp)

# OpenGL
if (XCI_WITH_OPENGL)
    find_package(glfw3 3.2 REQUIRED)
endif ()

# SFML
if (XCI_WITH_SFML)
    find_package(SFML 2.4 REQUIRED system window graphics)
endif ()


# Panda3D
if (XCI_WITH_PANDA)
    find_package(PythonLibs 2.7 REQUIRED)
    find_package(Panda3D REQUIRED)
endif ()

# Catch
find_package(Catch)

# PEGTL
find_package(PEGTL REQUIRED)

# Doxygen
find_package(Doxygen)

# TInfo (ncurses)
if (XCI_WITH_TINFO)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(TINFO REQUIRED tinfo)
endif ()

# Generate config.h
configure_file(config.h.in include/xci/config.h)

# ------------ #
# Libraries    #
# ------------ #

set(XCI_INCLUDE_DIRS
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>)

# Setup RPATH
if (APPLE)
    set(CMAKE_INSTALL_RPATH "@executable_path/../lib")
else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()

set(XCI_COMPAT_LIBRARY xci-compat)
add_subdirectory(src/xci/compat)

set(XCI_UTIL_LIBRARY xci-util)
add_subdirectory(src/xci/util)

if (XCI_WITH_OPENGL)
    set(XCI_GRAPHICS_OPENGL_LIBRARY xci-graphics-opengl)
    add_library(glad OBJECT ext/glad/glad.c)
    add_library(${XCI_GRAPHICS_OPENGL_LIBRARY}
        ${XCI_GRAPHICS_SOURCES} ${XCI_GRAPHICS_OPENGL_SOURCES}
        $<TARGET_OBJECTS:glad>)
    target_include_directories(${XCI_GRAPHICS_OPENGL_LIBRARY}
        PUBLIC
            ${XCI_INCLUDE_DIRS}
        PRIVATE
            ext)
    target_link_libraries(${XCI_GRAPHICS_OPENGL_LIBRARY}
        ${XCI_UTIL_LIBRARY}
        ${CMAKE_DL_LIBS}
        glfw)

    if (BUILD_FRAMEWORKS)
        set_target_properties(${XCI_GRAPHICS_OPENGL_LIBRARY} PROPERTIES
            FRAMEWORK TRUE
            FRAMEWORK_VERSION A
            MACOSX_FRAMEWORK_IDENTIFIER cz.xci.xci-graphics-opengl
            # "current version" in semantic format in Mach-O binary file
            VERSION 1.0.0
            # "compatibility version" in semantic format in Mach-O binary file
            SOVERSION 1.0.0
            PUBLIC_HEADER xci-graphics.h
            )
    endif()

    set(XCI_GRAPHICS_LIBRARY ${XCI_GRAPHICS_OPENGL_LIBRARY})
    install(TARGETS ${XCI_GRAPHICS_OPENGL_LIBRARY}
        EXPORT xcikit DESTINATION lib)
endif ()

if (XCI_WITH_SFML)
    set(XCI_GRAPHICS_SFML_LIBRARY xci-graphics-sfml)
    add_library(${XCI_GRAPHICS_SFML_LIBRARY}
        ${XCI_GRAPHICS_SOURCES} ${XCI_GRAPHICS_SFML_SOURCES})
    target_include_directories(${XCI_GRAPHICS_SFML_LIBRARY}
        PUBLIC
            ${XCI_INCLUDE_DIRS}
        PRIVATE
            ${SFML_INCLUDE_DIR})
    target_link_libraries(${XCI_GRAPHICS_SFML_LIBRARY}
        ${XCI_UTIL_LIBRARY}
        ${SFML_LIBRARIES})
    if (NOT DEFINED XCI_GRAPHICS_LIBRARY)
        set(XCI_GRAPHICS_LIBRARY ${XCI_GRAPHICS_SFML_LIBRARY})
    endif()
    install(TARGETS ${XCI_GRAPHICS_SFML_LIBRARY}
        EXPORT xcikit DESTINATION lib)
endif ()

if (XCI_WITH_PANDA)
    set(XCI_GRAPHICS_PANDA_LIBRARY xci-graphics-panda)
    add_library(${XCI_GRAPHICS_PANDA_LIBRARY}
        ${XCI_GRAPHICS_SOURCES} ${XCI_GRAPHICS_PANDA_SOURCES})
    target_include_directories(${XCI_GRAPHICS_PANDA_LIBRARY}
        PUBLIC
            ${XCI_INCLUDE_DIRS}
        PUBLIC
            ${PANDA3D_INCLUDE_DIRS}
            ${PYTHON_INCLUDE_DIRS})
    target_link_libraries(${XCI_GRAPHICS_PANDA_LIBRARY}
        ${XCI_UTIL_LIBRARY}
        ${PANDA3D_LIBRARIES})
    if (NOT DEFINED XCI_GRAPHICS_LIBRARY)
        set(XCI_GRAPHICS_LIBRARY ${XCI_GRAPHICS_PANDA_LIBRARY})
    endif()
    install(TARGETS ${XCI_GRAPHICS_PANDA_LIBRARY}
        EXPORT xcikit DESTINATION lib)
endif ()

set(XCI_TEXT_LIBRARY xci-text)
add_library(${XCI_TEXT_LIBRARY} ${XCI_TEXT_SOURCES}
    $<TARGET_OBJECTS:${RBP_LIBRARY}>)
target_include_directories(${XCI_TEXT_LIBRARY}
    PUBLIC
        ${XCI_INCLUDE_DIRS}
        ${FREETYPE_INCLUDE_DIRS}
        ${PEGTL_INCLUDE_DIRS}
    PRIVATE
        ext)
target_link_libraries(${XCI_TEXT_LIBRARY}
    ${XCI_GRAPHICS_LIBRARY}
    ${XCI_UTIL_LIBRARY}
    ${FREETYPE_LIBRARIES})

if (BUILD_FRAMEWORKS)
    set_target_properties(${XCI_TEXT_LIBRARY} PROPERTIES
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

install(TARGETS ${XCI_TEXT_LIBRARY} EXPORT xcikit DESTINATION lib)

set(XCI_WIDGETS_LIBRARY xci-widgets)
add_library(${XCI_WIDGETS_LIBRARY} ${XCI_WIDGETS_SOURCES})
target_link_libraries(${XCI_WIDGETS_LIBRARY}
    ${XCI_TEXT_LIBRARY}
    ${XCI_GRAPHICS_LIBRARY})

if (BUILD_FRAMEWORKS)
    set_target_properties(${XCI_WIDGETS_LIBRARY} PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER cz.xci.xci-widgets-opengl
        # "current version" in semantic format in Mach-O binary file
        VERSION 1.0.0
        # "compatibility version" in semantic format in Mach-O binary file
        SOVERSION 1.0.0
        PUBLIC_HEADER xci-widgets.h
        )
endif()

install(TARGETS ${XCI_WIDGETS_LIBRARY} EXPORT xcikit DESTINATION lib)


# ------------ #
# Examples     #
# ------------ #

add_subdirectory(examples)


# ------------ #
# Tests        #
# ------------ #

if (Catch_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif ()


# ------------ #
# Benchmarks   #
# ------------ #

if (XCI_BENCHMARK)
    # Google Benchmark
    find_package(benchmark REQUIRED)
    add_subdirectory(benchmarks)
endif ()


# ------------- #
# Documentation #
# ------------- #

# Only with CMake v3.9+ (older versions don't have doxygen_add_docs())
if (DOXYGEN_FOUND AND NOT CMAKE_VERSION VERSION_LESS "3.9")
    set(DOXYGEN_OUTPUT_DIRECTORY doc)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
    set(DOXYGEN_SOURCE_BROWSER YES)
    set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
    set(DOXYGEN_SORT_MEMBER_DOCS NO)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
    doxygen_add_docs(doc
        src/xci
        docs
        README.md
        COMMENT "Generate documentation")
endif ()


# ------------- #
# Installation  #
# ------------- #

install(DIRECTORY src/xci DESTINATION include
    FILES_MATCHING
    REGEX "xci/(graphics|util)/[a-z]+$" EXCLUDE  # internal headers
    PATTERN "*.h")
install(DIRECTORY ext/nonstd DESTINATION include/xci/compat)

# config.h
install(DIRECTORY ${CMAKE_BINARY_DIR}/include/xci DESTINATION include)

install(EXPORT xcikit
    NAMESPACE xcikit::
    DESTINATION lib/cmake/xci
    FILE "xcikitTargets.cmake")

# Make our CMake modules available to clients
install(FILES cmake-modules/FindCatch.cmake DESTINATION lib/cmake/xci)

set(XCI_INCLUDE_DIRS include)
include(CMakePackageConfigHelpers)
configure_package_config_file(xcikitConfig.cmake.in xcikitConfig.cmake
    INSTALL_DESTINATION lib/cmake/xci
    PATH_VARS XCI_INCLUDE_DIRS)
write_basic_package_version_file(xcikitConfigVersion.cmake
    VERSION 0.1.0
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/xcikitConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/xcikitConfigVersion.cmake
    DESTINATION lib/cmake/xci)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules/XciBuildOptions.cmake
    DESTINATION lib/cmake/xci)
