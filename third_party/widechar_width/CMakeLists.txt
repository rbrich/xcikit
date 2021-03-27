cmake_minimum_required(VERSION 3.13)

add_library(widechar_width OBJECT widechar_width.cpp)
add_library(third_party::widechar_width ALIAS widechar_width)

target_include_directories(widechar_width INTERFACE ${CMAKE_CURRENT_LIST_DIR}/..)
set_target_properties(widechar_width PROPERTIES CXX_CLANG_TIDY "")
