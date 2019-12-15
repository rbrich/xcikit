set(INST_EXAMPLES share/xcikit/examples)

add_subdirectory(plugin)
add_subdirectory(script)

if (XCI_WITH_VULKAN)
    add_subdirectory(graphics)
    add_subdirectory(widgets)
endif()

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

# text

if (XCI_WITH_VULKAN)
    add_executable(demo_font demo_font.cpp)
    target_link_libraries(demo_font xci-text xci-graphics)
    install(TARGETS demo_font EXPORT xcikit DESTINATION ${INST_EXAMPLES})

    add_executable(demo_layout demo_layout.cpp)
    target_link_libraries(demo_layout xci-text xci-graphics)
    install(TARGETS demo_layout EXPORT xcikit DESTINATION ${INST_EXAMPLES})
endif()
