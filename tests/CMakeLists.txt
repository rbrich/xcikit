add_executable(test_util test_util.cpp test_main.cpp)
target_link_libraries(test_util Catch::Catch ${XCI_UTIL_LIBRARY})
add_test(NAME test_util COMMAND test_util)
