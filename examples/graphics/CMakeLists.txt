add_executable(demo_vulkan demo_vulkan.cpp)
target_link_libraries(demo_vulkan xci-graphics)
install(TARGETS demo_vulkan EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_coords demo_coords.cpp)
target_link_libraries(demo_coords xci-text xci-graphics)
install(TARGETS demo_coords EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_fps demo_fps.cpp)
target_link_libraries(demo_fps xci-widgets xci-text xci-graphics)
install(TARGETS demo_fps EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_shapes demo_shapes.cpp)
target_link_libraries(demo_shapes xci-text xci-graphics)
install(TARGETS demo_shapes EXPORT xcikit DESTINATION ${INST_EXAMPLES})
