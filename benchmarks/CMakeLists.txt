add_executable(bm_core bm_core.cpp)
target_link_libraries(bm_core benchmark::benchmark ${XCI_CORE_LIBRARY})
