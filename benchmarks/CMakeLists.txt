add_executable(bm_core bm_core.cpp)
target_link_libraries(bm_core benchmark::benchmark xci-core)
install(TARGETS bm_core EXPORT xcikit DESTINATION benchmarks)

add_executable(bm_chunked_stack bm_chunked_stack.cpp)
target_link_libraries(bm_chunked_stack benchmark::benchmark xci-core)
install(TARGETS bm_chunked_stack EXPORT xcikit DESTINATION benchmarks)

if (XCI_SCRIPT)
    add_executable(bm_script bm_script.cpp)
    target_link_libraries(bm_script benchmark::benchmark xci-script)
    install(TARGETS bm_script EXPORT xcikit DESTINATION benchmarks)
endif()
