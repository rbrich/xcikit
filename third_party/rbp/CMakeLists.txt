cmake_minimum_required(VERSION 3.13)

add_library(rbp OBJECT
    MaxRectsBinPack.cpp
    MaxRectsBinPack.h
    Rect.cpp
    Rect.h
)
add_library(third_party::rbp ALIAS rbp)

target_include_directories(rbp INTERFACE ${CMAKE_CURRENT_LIST_DIR}/..)
set_target_properties(rbp PROPERTIES CXX_CLANG_TIDY "")
