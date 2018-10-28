add_executable(test_core test_core.cpp)
target_link_libraries(test_core Catch::Catch xci-core)
add_test(NAME test_core COMMAND test_core)

add_executable(test_graphics test_graphics.cpp)
target_link_libraries(test_graphics Catch::Catch xci-graphics)
add_test(NAME test_graphics COMMAND test_graphics)

add_executable(test_widgets test_widgets.cpp)
target_link_libraries(test_widgets Catch::Catch xci-widgets xci-text xci-graphics)
add_test(NAME test_widgets COMMAND test_widgets)
