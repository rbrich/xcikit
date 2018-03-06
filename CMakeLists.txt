cmake_minimum_required(VERSION 3.5)
project(xcikit)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wno-reorder")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)

option(WITH_SFML "Link with SFML and build libxci-graphics-sfml." ON)
option(WITH_PANDA "Link with Panda3D and build libxci-graphics-panda." OFF)

# ------------ #
# Sources      #
# ------------ #

set(XCI_UTIL_SOURCES
    src/xci/util/log.cpp
    src/xci/util/log.h
    src/xci/util/format.h
    src/xci/util/geometry.h)

set(XCI_GRAPHICS_SOURCES
    src/xci/graphics/Color.h
    src/xci/graphics/Texture.h
    src/xci/graphics/Sprites.h
    src/xci/graphics/View.h
    src/xci/graphics/Window.h)

set(XCI_GRAPHICS_SFML_SOURCES
    src/xci/graphics/sfml/Texture.cpp
    src/xci/graphics/sfml/TextureImpl.h
    src/xci/graphics/sfml/Sprites.cpp
    src/xci/graphics/sfml/SpritesImpl.h
    src/xci/graphics/sfml/View.cpp
    src/xci/graphics/sfml/ViewImpl.h
    src/xci/graphics/sfml/Window.cpp
    src/xci/graphics/sfml/WindowImpl.h)

set(XCI_GRAPHICS_PANDA_SOURCES
    src/xci/graphics/panda/Texture.cpp
    src/xci/graphics/panda/TextureImpl.h
    src/xci/graphics/panda/Sprites.cpp
    src/xci/graphics/panda/SpritesImpl.h
    src/xci/graphics/panda/View.cpp
    src/xci/graphics/panda/ViewImpl.h
    src/xci/graphics/panda/Window.cpp
    src/xci/graphics/panda/WindowImpl.h)

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
    src/xci/text/Text.h)


# ------------ #
# Dependencies #
# ------------ #

# FreeType
include(FindFreetype)

# RectangleBinPack
add_subdirectory(src/rbp)
set(RBP_LIBRARY rbp)

# SFML
if (WITH_SFML)
    find_package(SFML 2.4 REQUIRED system window graphics)
endif (WITH_SFML)


# Panda3D
if (WITH_PANDA)
    find_package(PythonLibs 2.7 REQUIRED)
    find_package(Panda3D REQUIRED)
endif (WITH_PANDA)

# ------------ #
# Libraries    #
# ------------ #

set(XCI_UTIL_LIBRARY xci-util)
add_library(${XCI_UTIL_LIBRARY} STATIC ${XCI_UTIL_SOURCES})
target_include_directories(${XCI_UTIL_LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

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
        ${PANDA3D_LIBRARIES})
    if (NOT DEFINED XCI_GRAPHICS_LIBRARY)
        set(XCI_GRAPHICS_LIBRARY ${XCI_GRAPHICS_PANDA_LIBRARY})
    endif()
endif (WITH_PANDA)

set(XCI_TEXT_LIBRARY xci-text)
add_library(${XCI_TEXT_LIBRARY} STATIC ${XCI_TEXT_SOURCES})
target_include_directories(${XCI_TEXT_LIBRARY} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(${XCI_TEXT_LIBRARY}
    ${XCI_UTIL_LIBRARY}
    ${FREETYPE_LIBRARIES}
    ${RBP_LIBRARY})


# ------------ #
# Examples     #
# ------------ #

add_executable(demo_logger examples/demo_logger.cpp)
target_link_libraries(demo_logger ${XCI_UTIL_LIBRARY})

add_executable(demo_thread examples/demo_thread.cpp)
target_link_libraries(demo_thread ${XCI_TEXT_LIBRARY} pthread)

add_executable(demo_font examples/demo_font.cpp)
target_link_libraries(demo_font
    ${XCI_GRAPHICS_LIBRARY}
    ${XCI_TEXT_LIBRARY}
    pthread)
