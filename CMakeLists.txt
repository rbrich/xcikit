cmake_minimum_required(VERSION 3.5)
project(xcikit C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wno-reorder")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)

if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
endif()

option(WITH_OPENGL "Link with GLFW and build libxci-graphics-opengl." ON)
option(WITH_SFML "Link with SFML and build libxci-graphics-sfml." OFF)
option(WITH_PANDA "Link with Panda3D and build libxci-graphics-panda." OFF)

option(DEBUG_MARKUP_DUMP_TOKENS "Text markup parser debugging." OFF)
option(DEBUG_OPENGL "Log info about OpenGL calls and errors." OFF)

set(SHARE_DIR "${CMAKE_CURRENT_LIST_DIR}/share" CACHE PATH "Location of share dir (runtime data)")

# ------------ #
# Sources      #
# ------------ #

set(XCI_UTIL_SOURCES
    src/xci/util/compat.h
    src/xci/util/file.cpp
    src/xci/util/file.h
    src/xci/util/FileWatch.cpp
    src/xci/util/FileWatch.h
    src/xci/util/format.cpp
    src/xci/util/format.h
    src/xci/util/geometry.h
    src/xci/util/log.cpp
    src/xci/util/log.h
    src/xci/util/string.cpp
    src/xci/util/string.h)

set(XCI_GRAPHICS_SOURCES
    src/xci/graphics/Color.h
    src/xci/graphics/Shapes.h
    src/xci/graphics/Shapes.inl
    src/xci/graphics/Texture.h
    src/xci/graphics/Texture.inl
    src/xci/graphics/Sprites.h
    src/xci/graphics/Sprites.inl
    src/xci/graphics/View.h
    src/xci/graphics/View.inl
    src/xci/graphics/Window.h
    src/xci/graphics/Window.inl)

set(XCI_GRAPHICS_OPENGL_SOURCES
    src/xci/graphics/opengl/GlEllipses.cpp
    src/xci/graphics/opengl/GlEllipses.h
    src/xci/graphics/opengl/GlRectangles.cpp
    src/xci/graphics/opengl/GlRectangles.h
    src/xci/graphics/opengl/GlShapes.cpp
    src/xci/graphics/opengl/GlShapes.h
    src/xci/graphics/opengl/GlSprites.cpp
    src/xci/graphics/opengl/GlSprites.h
    src/xci/graphics/opengl/GlTexture.cpp
    src/xci/graphics/opengl/GlTexture.h
    src/xci/graphics/opengl/GlView.cpp
    src/xci/graphics/opengl/GlView.h
    src/xci/graphics/opengl/GlWindow.cpp
    src/xci/graphics/opengl/GlWindow.h)

set(XCI_GRAPHICS_SFML_SOURCES
    src/xci/graphics/sfml/SfmlTexture.cpp
    src/xci/graphics/sfml/SfmlTexture.h
    src/xci/graphics/sfml/SfmlSprites.cpp
    src/xci/graphics/sfml/SfmlSprites.h
    src/xci/graphics/sfml/SfmlView.cpp
    src/xci/graphics/sfml/SfmlView.h
    src/xci/graphics/sfml/SfmlWindow.cpp
    src/xci/graphics/sfml/SfmlWindow.h)

set(XCI_GRAPHICS_PANDA_SOURCES
    src/xci/graphics/panda/PandaTexture.cpp
    src/xci/graphics/panda/PandaTexture.h
    src/xci/graphics/panda/PandaSprites.cpp
    src/xci/graphics/panda/PandaSprites.h
    src/xci/graphics/panda/PandaView.cpp
    src/xci/graphics/panda/PandaView.h
    src/xci/graphics/panda/PandaWindow.cpp
    src/xci/graphics/panda/PandaWindow.h)

set(XCI_TEXT_SOURCES
    src/xci/text/FontLibrary.cpp
    src/xci/text/FontLibrary.h
    src/xci/text/FontFace.cpp
    src/xci/text/FontFace.h
    src/xci/text/FontTexture.cpp
    src/xci/text/FontTexture.h
    src/xci/text/Font.cpp
    src/xci/text/Font.h
    src/xci/text/Text.cpp
    src/xci/text/Text.h
    src/xci/text/Layout.cpp
    src/xci/text/Layout.h
    src/xci/text/Markup.cpp
    src/xci/text/Markup.h
    src/xci/text/Style.cpp
    src/xci/text/Style.h
    src/xci/text/layout/Page.cpp
    src/xci/text/layout/Page.h
    src/xci/text/layout/Element.h)

set(XCI_WIDGETS_SOURCES
    src/xci/widgets/Button.cpp
    src/xci/widgets/Button.h)

# ------------ #
# Dependencies #
# ------------ #

# pthread
find_package(Threads REQUIRED)

# FreeType
include(FindFreetype)

# RectangleBinPack
add_subdirectory(src/rbp)
set(RBP_LIBRARY rbp)

# OpenGL
if (WITH_OPENGL)
    find_package(glfw3 3.2 REQUIRED)
endif (WITH_OPENGL)

# SFML
if (WITH_SFML)
    find_package(SFML 2.4 REQUIRED system window graphics)
endif (WITH_SFML)


# Panda3D
if (WITH_PANDA)
    find_package(PythonLibs 2.7 REQUIRED)
    find_package(Panda3D REQUIRED)
endif (WITH_PANDA)

# Catch
find_package(Catch)

# PEGTL
find_package(PEGTL REQUIRED)


# ------------ #
# Libraries    #
# ------------ #

set(XCI_UTIL_LIBRARY xci-util)
add_library(${XCI_UTIL_LIBRARY} STATIC ${XCI_UTIL_SOURCES})
target_link_libraries(${XCI_UTIL_LIBRARY} Threads::Threads)
target_include_directories(${XCI_UTIL_LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(${XCI_UTIL_LIBRARY} PRIVATE XCI_SHARE_DIR="${SHARE_DIR}")

if (WITH_OPENGL)
    set(XCI_GRAPHICS_OPENGL_LIBRARY xci-graphics-opengl)
    add_library(glad STATIC src/glad/glad.c src/glad/glad.h)
    add_library(${XCI_GRAPHICS_OPENGL_LIBRARY} STATIC
        ${XCI_GRAPHICS_SOURCES} ${XCI_GRAPHICS_OPENGL_SOURCES})
    target_include_directories(${XCI_GRAPHICS_OPENGL_LIBRARY}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(${XCI_GRAPHICS_OPENGL_LIBRARY}
        ${XCI_UTIL_LIBRARY}
        dl glad glfw)
    if (DEBUG_OPENGL)
        target_compile_definitions(${XCI_GRAPHICS_OPENGL_LIBRARY} PRIVATE
            XCI_DEBUG_OPENGL)
    endif (DEBUG_OPENGL)
    set(XCI_GRAPHICS_LIBRARY ${XCI_GRAPHICS_OPENGL_LIBRARY})
endif (WITH_OPENGL)

if (WITH_SFML)
    set(XCI_GRAPHICS_SFML_LIBRARY xci-graphics-sfml)
    add_library(${XCI_GRAPHICS_SFML_LIBRARY} STATIC
        ${XCI_GRAPHICS_SOURCES} ${XCI_GRAPHICS_SFML_SOURCES})
    target_include_directories(${XCI_GRAPHICS_SFML_LIBRARY}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
        PRIVATE
            ${SFML_INCLUDE_DIR})
    target_link_libraries(${XCI_GRAPHICS_SFML_LIBRARY}
        ${XCI_UTIL_LIBRARY}
        ${SFML_LIBRARIES})
    set(XCI_GRAPHICS_LIBRARY ${XCI_GRAPHICS_SFML_LIBRARY})
endif (WITH_SFML)

if (WITH_PANDA)
    set(XCI_GRAPHICS_PANDA_LIBRARY xci-graphics-panda)
    add_library(${XCI_GRAPHICS_PANDA_LIBRARY} STATIC
        ${XCI_GRAPHICS_SOURCES} ${XCI_GRAPHICS_PANDA_SOURCES})
    target_include_directories(${XCI_GRAPHICS_PANDA_LIBRARY}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        PUBLIC
        ${PANDA3D_INCLUDE_DIRS}
        ${PYTHON_INCLUDE_DIRS})
    target_link_libraries(${XCI_GRAPHICS_PANDA_LIBRARY}
        ${XCI_UTIL_LIBRARY}
        ${PANDA3D_LIBRARIES})
    if (NOT DEFINED XCI_GRAPHICS_LIBRARY)
        set(XCI_GRAPHICS_LIBRARY ${XCI_GRAPHICS_PANDA_LIBRARY})
    endif()
endif (WITH_PANDA)

set(XCI_TEXT_LIBRARY xci-text)
add_library(${XCI_TEXT_LIBRARY} STATIC ${XCI_TEXT_SOURCES})
target_include_directories(${XCI_TEXT_LIBRARY} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FREETYPE_INCLUDE_DIRS}
    ${PEGTL_INCLUDE_DIRS})
target_link_libraries(${XCI_TEXT_LIBRARY}
    ${XCI_GRAPHICS_LIBRARY}
    ${XCI_UTIL_LIBRARY}
    ${FREETYPE_LIBRARIES}
    ${RBP_LIBRARY})

if (DEBUG_MARKUP_DUMP_TOKENS)
    target_compile_definitions(${XCI_TEXT_LIBRARY} PRIVATE
        XCI_DEBUG_MARKUP_DUMP_TOKENS)
endif (DEBUG_MARKUP_DUMP_TOKENS)

set(XCI_WIDGETS_LIBRARY xci-widgets)
add_library(${XCI_WIDGETS_LIBRARY} STATIC ${XCI_WIDGETS_SOURCES})
target_link_libraries(${XCI_WIDGETS_LIBRARY}
    ${XCI_TEXT_LIBRARY}
    ${XCI_GRAPHICS_LIBRARY})

# ------------ #
# Examples     #
# ------------ #

add_subdirectory(examples)


# ------------ #
# Tests        #
# ------------ #

if (CATCH_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif (CATCH_FOUND)
