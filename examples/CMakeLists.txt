set(INST_EXAMPLES share/xcikit/examples)

add_subdirectory(plugin)
add_subdirectory(script)

# core

add_executable(demo_log demo_log.cpp)
target_link_libraries(demo_log xci-core)
install(TARGETS demo_log EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_filewatch demo_filewatch.cpp)
target_link_libraries(demo_filewatch xci-core)
install(TARGETS demo_filewatch EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_termctl demo_termctl.cpp)
target_link_libraries(demo_termctl xci-core)
install(TARGETS demo_termctl EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_vfs demo_vfs.cpp)
target_link_libraries(demo_vfs xci-core)
install(TARGETS demo_vfs EXPORT xcikit DESTINATION ${INST_EXAMPLES})

# data

add_executable(demo_reflect reflect/demo_reflect.cpp)
target_link_libraries(demo_reflect xci-data)

# graphics

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

# text

add_executable(demo_font demo_font.cpp)
target_link_libraries(demo_font xci-text xci-graphics)
install(TARGETS demo_font EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_layout demo_layout.cpp)
target_link_libraries(demo_layout xci-text xci-graphics)
install(TARGETS demo_layout EXPORT xcikit DESTINATION ${INST_EXAMPLES})

# widgets

add_subdirectory(widgets)
