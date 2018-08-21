add_executable(bm_util bm_util.cpp)
target_link_libraries(bm_util benchmark::benchmark ${XCI_UTIL_LIBRARY})
