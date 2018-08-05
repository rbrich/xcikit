add_executable(test_util test_util.cpp test_main.cpp)
target_link_libraries(test_util Catch::Catch ${XCI_UTIL_LIBRARY})
add_test(NAME test_util COMMAND test_util)

add_executable(test_graphics test_graphics.cpp test_main.cpp)
target_link_libraries(test_graphics Catch::Catch ${XCI_GRAPHICS_LIBRARY})
add_test(NAME test_graphics COMMAND test_graphics)
