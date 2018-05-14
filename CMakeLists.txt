cmake_minimum_required(VERSION 3.5)
project(xcikit C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wno-reorder")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)

if (EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
endif()

option(WITH_OPENGL "Link with GLFW and build libxci-graphics-opengl." ON)
option(WITH_SFML "Link with SFML and build libxci-graphics-sfml." OFF)
option(WITH_PANDA "Link with Panda3D and build libxci-graphics-panda." OFF)

option(EMBED_SHADERS "Embed shader files into binaries to avoid dependency on share dir." OFF)

option(DEBUG_CLANG_ASAN "Build with Clang address sanitizer." OFF)
option(DEBUG_MARKUP_DUMP_TOKENS "Text markup parser debugging." OFF)
option(DEBUG_OPENGL "Log info about OpenGL calls and errors." OFF)

set(SHARE_DIR "${CMAKE_CURRENT_LIST_DIR}/share" CACHE PATH "Location of share dir (runtime data)")

if (DEBUG_CLANG_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif (DEBUG_CLANG_ASAN)

# ------------ #
# Sources      #
# ------------ #

set(XCI_GRAPHICS_SOURCES
    src/xci/graphics/Color.h
    src/xci/graphics/Primitives.h
    src/xci/graphics/Renderer.cpp
    src/xci/graphics/Renderer.h
    src/xci/graphics/Shader.h
    src/xci/graphics/Shape.cpp
    src/xci/graphics/Shape.h
    src/xci/graphics/Sprites.cpp
    src/xci/graphics/Sprites.h
    src/xci/graphics/Texture.h
    src/xci/graphics/View.cpp
    src/xci/graphics/View.h
    src/xci/graphics/Window.cpp
    src/xci/graphics/Window.h)

set(XCI_GRAPHICS_OPENGL_SOURCES
    src/xci/graphics/opengl/GlPrimitives.cpp
    src/xci/graphics/opengl/GlPrimitives.h
    src/xci/graphics/opengl/GlRenderer.cpp
    src/xci/graphics/opengl/GlRenderer.h
    src/xci/graphics/opengl/GlShader.cpp
    src/xci/graphics/opengl/GlShader.h
    src/xci/graphics/opengl/GlTexture.cpp
    src/xci/graphics/opengl/GlTexture.h
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
    src/xci/widgets/Button.h
    src/xci/widgets/Checkbox.cpp
    src/xci/widgets/Checkbox.h
    src/xci/widgets/FpsDisplay.cpp
    src/xci/widgets/FpsDisplay.h
    src/xci/widgets/Icon.cpp
    src/xci/widgets/Icon.h
    src/xci/widgets/Theme.cpp
    src/xci/widgets/Theme.h
    src/xci/widgets/Widget.cpp
    src/xci/widgets/Widget.h)


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

# Doxygen
find_package(Doxygen)


# ------------ #
# Libraries    #
# ------------ #

set(XCI_BUILD_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/src)
set(XCI_INCLUDE_DIRS include)

set(XCI_UTIL_LIBRARY xci-util)
add_subdirectory(src/xci/util)

if (WITH_OPENGL)
    set(XCI_GRAPHICS_OPENGL_LIBRARY xci-graphics-opengl)
    add_library(glad OBJECT src/glad/glad.c src/glad/glad.h)
    add_library(${XCI_GRAPHICS_OPENGL_LIBRARY} STATIC
        ${XCI_GRAPHICS_SOURCES} ${XCI_GRAPHICS_OPENGL_SOURCES}
        $<TARGET_OBJECTS:glad>)
    target_include_directories(${XCI_GRAPHICS_OPENGL_LIBRARY}
        PUBLIC
            $<BUILD_INTERFACE:${XCI_BUILD_INCLUDE_DIRS}>
            $<INSTALL_INTERFACE:${XCI_INCLUDE_DIRS}>
        PRIVATE ext/incbin)
    target_link_libraries(${XCI_GRAPHICS_OPENGL_LIBRARY}
        ${XCI_UTIL_LIBRARY}
        ${CMAKE_DL_LIBS}
        glfw)
    target_compile_definitions(${XCI_GRAPHICS_OPENGL_LIBRARY} PRIVATE
        XCI_WITH_OPENGL
        XCI_SHARE_DIR="${SHARE_DIR}"
        $<$<BOOL:${EMBED_SHADERS}>:XCI_EMBED_SHADERS>
        $<$<BOOL:${DEBUG_OPENGL}>:XCI_DEBUG_OPENGL>)
    set(XCI_GRAPHICS_LIBRARY ${XCI_GRAPHICS_OPENGL_LIBRARY})
    install(TARGETS ${XCI_GRAPHICS_OPENGL_LIBRARY}
        EXPORT xcikit DESTINATION lib)
endif (WITH_OPENGL)

if (WITH_SFML)
    set(XCI_GRAPHICS_SFML_LIBRARY xci-graphics-sfml)
    add_library(${XCI_GRAPHICS_SFML_LIBRARY} STATIC
        ${XCI_GRAPHICS_SOURCES} ${XCI_GRAPHICS_SFML_SOURCES})
    target_include_directories(${XCI_GRAPHICS_SFML_LIBRARY}
        PUBLIC
            $<BUILD_INTERFACE:${XCI_BUILD_INCLUDE_DIRS}>
            $<INSTALL_INTERFACE:${XCI_INCLUDE_DIRS}>
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
endif (WITH_SFML)

if (WITH_PANDA)
    set(XCI_GRAPHICS_PANDA_LIBRARY xci-graphics-panda)
    add_library(${XCI_GRAPHICS_PANDA_LIBRARY} STATIC
        ${XCI_GRAPHICS_SOURCES} ${XCI_GRAPHICS_PANDA_SOURCES})
    target_include_directories(${XCI_GRAPHICS_PANDA_LIBRARY}
        PUBLIC
            $<BUILD_INTERFACE:${XCI_BUILD_INCLUDE_DIRS}>
            $<INSTALL_INTERFACE:${XCI_INCLUDE_DIRS}>
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
endif (WITH_PANDA)

set(XCI_TEXT_LIBRARY xci-text)
add_library(${XCI_TEXT_LIBRARY} STATIC ${XCI_TEXT_SOURCES}
    $<TARGET_OBJECTS:${RBP_LIBRARY}>)
target_include_directories(${XCI_TEXT_LIBRARY} PUBLIC
    $<BUILD_INTERFACE:${XCI_BUILD_INCLUDE_DIRS}>
    $<INSTALL_INTERFACE:${XCI_INCLUDE_DIRS}>
    ${FREETYPE_INCLUDE_DIRS}
    ${PEGTL_INCLUDE_DIRS})
target_link_libraries(${XCI_TEXT_LIBRARY}
    ${XCI_GRAPHICS_LIBRARY}
    ${XCI_UTIL_LIBRARY}
    ${FREETYPE_LIBRARIES})
install(TARGETS ${XCI_TEXT_LIBRARY} EXPORT xcikit DESTINATION lib)

if (DEBUG_MARKUP_DUMP_TOKENS)
    target_compile_definitions(${XCI_TEXT_LIBRARY} PRIVATE
        XCI_DEBUG_MARKUP_DUMP_TOKENS)
endif (DEBUG_MARKUP_DUMP_TOKENS)

set(XCI_WIDGETS_LIBRARY xci-widgets)
add_library(${XCI_WIDGETS_LIBRARY} STATIC ${XCI_WIDGETS_SOURCES})
target_link_libraries(${XCI_WIDGETS_LIBRARY}
    ${XCI_TEXT_LIBRARY}
    ${XCI_GRAPHICS_LIBRARY})
target_compile_definitions(${XCI_WIDGETS_LIBRARY} PRIVATE
    XCI_SHARE_DIR="${SHARE_DIR}")
install(TARGETS ${XCI_WIDGETS_LIBRARY} EXPORT xcikit DESTINATION lib)


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



# ------------- #
# Documentation #
# ------------- #

if (DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIRECTORY doc)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
    set(DOXYGEN_SOURCE_BROWSER YES)
    set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
    set(DOXYGEN_SORT_MEMBER_DOCS NO)
    doxygen_add_docs(doc
        src/xci
        docs
        README.md
        COMMENT "Generate documentation")
endif (DOXYGEN_FOUND)


# ------------- #
# Installation  #
# ------------- #

install(DIRECTORY src/xci DESTINATION include
    FILES_MATCHING
    REGEX "xci/[a-z]+/[a-z]+$" EXCLUDE
    REGEX "xci/[a-z]+/[^/]+\\.h")

install(EXPORT xcikit DESTINATION lib/cmake/xci FILE "xcikitTargets.cmake")

include(CMakePackageConfigHelpers)
configure_package_config_file(xcikitConfig.cmake.in xcikitConfig.cmake
    INSTALL_DESTINATION lib/cmake/xci
    PATH_VARS XCI_INCLUDE_DIRS)
write_basic_package_version_file(xcikitConfigVersion.cmake
    VERSION 0.0.1
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/xcikitConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/xcikitConfigVersion.cmake
    DESTINATION lib/cmake/xci)
