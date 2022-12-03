add_library(widechar_width OBJECT widechar_width.cpp)
add_library(third_party::widechar_width ALIAS widechar_width)

target_compile_features(widechar_width PUBLIC cxx_std_${XCI_CXX_STANDARD})

target_include_directories(widechar_width INTERFACE ${CMAKE_CURRENT_LIST_DIR}/..)
set_target_properties(widechar_width PROPERTIES CXX_CLANG_TIDY "")
