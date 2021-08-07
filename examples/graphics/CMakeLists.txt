add_executable(demo_vulkan demo_vulkan.cpp)
target_link_libraries(demo_vulkan xci-graphics)
add_dependencies(demo_vulkan share)
install(TARGETS demo_vulkan EXPORT xcikit DESTINATION examples/graphics)

add_executable(demo_coords demo_coords.cpp)
target_link_libraries(demo_coords xci-text xci-graphics)
add_dependencies(demo_coords share)
install(TARGETS demo_coords EXPORT xcikit DESTINATION examples/graphics)

add_executable(demo_fps demo_fps.cpp)
target_link_libraries(demo_fps xci-widgets xci-text xci-graphics)
add_dependencies(demo_fps share)
install(TARGETS demo_fps EXPORT xcikit DESTINATION examples/graphics)

add_executable(demo_shapes demo_shapes.cpp)
target_link_libraries(demo_shapes xci-text xci-graphics)
add_dependencies(demo_shapes share)
install(TARGETS demo_shapes EXPORT xcikit DESTINATION examples/graphics)
