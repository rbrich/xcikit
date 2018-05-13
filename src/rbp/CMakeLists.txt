cmake_minimum_required(VERSION 3.5)

set(RBP_SOURCES
    MaxRectsBinPack.cpp
    MaxRectsBinPack.h
    Rect.cpp
    Rect.h)

set(RBP_LIBRARY rbp)
add_library(${RBP_LIBRARY} OBJECT ${RBP_SOURCES})
target_include_directories(${RBP_LIBRARY} INTERFACE .)
