add_executable(bm_core bm_core.cpp)
target_link_libraries(bm_core benchmark::benchmark xci-core)

add_executable(bm_stack bm_stack.cpp)
target_link_libraries(bm_stack benchmark::benchmark xci-core)
