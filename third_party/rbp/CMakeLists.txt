add_library(rbp OBJECT
    MaxRectsBinPack.cpp
    MaxRectsBinPack.h
    Rect.cpp
    Rect.h
)
add_library(third_party::rbp ALIAS rbp)

target_compile_features(rbp PUBLIC cxx_std_${XCI_CXX_STANDARD})

target_include_directories(rbp
    INTERFACE ${CMAKE_CURRENT_LIST_DIR}/..
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../src)
set_target_properties(rbp PROPERTIES CXX_CLANG_TIDY "")
