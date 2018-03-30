add_executable(test_graphics test_graphics.cpp test_main.cpp)
target_link_libraries(test_graphics ${XCI_GRAPHICS_LIBRARY})
target_include_directories(test_graphics PRIVATE ${CATCH_INCLUDE_DIRS})

add_executable(test_util test_util.cpp test_main.cpp)
target_link_libraries(test_util ${XCI_UTIL_LIBRARY})
target_include_directories(test_util PRIVATE ${CATCH_INCLUDE_DIRS})
