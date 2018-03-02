cmake_minimum_required(VERSION 3.9)
project(xcikit)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")


# ------------ #
# Sources      #
# ------------ #

set(XCI_LOG_SOURCES
    src/xci/log/Logger.cpp
    src/xci/log/Logger.h
    src/xci/util/format.h)

set(XCI_TEXT_SOURCES
    src/xci/text/FontLibrary.cpp
    src/xci/text/FontFace.cpp
    src/xci/text/FontLibrary.h
    src/xci/text/FontFace.h)


# ------------ #
# Dependencies #
# ------------ #

# FreeType
include(FindFreetype)

# RectangleBinPack
add_subdirectory(src/rbp)


# ------------ #
# Libraries    #
# ------------ #

set(XCI_LOG_LIBRARY xci-log)
add_library(${XCI_LOG_LIBRARY} STATIC ${XCI_LOG_SOURCES})
target_include_directories(${XCI_LOG_LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(XCI_TEXT_LIBRARY xci-text)
add_library(${XCI_TEXT_LIBRARY} STATIC ${XCI_TEXT_SOURCES})
target_include_directories(${XCI_TEXT_LIBRARY} PUBLIC ${FREETYPE_INCLUDE_DIRS})
target_include_directories(${XCI_TEXT_LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${XCI_TEXT_LIBRARY}
        ${XCI_LOG_LIBRARY}
        ${FREETYPE_LIBRARIES}
        ${RBP_LIBRARY})


# ------------ #
# Examples     #
# ------------ #

add_executable(demo_logger examples/demo_logger.cpp)
target_link_libraries(demo_logger ${XCI_LOG_LIBRARY})

add_executable(demo_font examples/demo_font.cpp)
target_link_libraries(demo_font ${XCI_TEXT_LIBRARY} pthread)
