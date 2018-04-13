add_executable(test_util test_util.cpp test_main.cpp)
target_link_libraries(test_util ${XCI_UTIL_LIBRARY})
target_include_directories(test_util PRIVATE ${CATCH_INCLUDE_DIRS})
add_test(NAME test_util COMMAND test_util)
