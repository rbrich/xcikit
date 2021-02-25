cmake_minimum_required(VERSION 3.15)

add_library(rbp OBJECT
    MaxRectsBinPack.cpp
    MaxRectsBinPack.h
    Rect.cpp
    Rect.h
    )

target_include_directories(rbp INTERFACE ${CMAKE_CURRENT_LIST_DIR}/..)
set_target_properties(rbp PROPERTIES CXX_CLANG_TIDY "")
