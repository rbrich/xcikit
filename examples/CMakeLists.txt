add_subdirectory(plugin)

if (XCI_WITH_VULKAN)
    add_subdirectory(graphics)
    add_subdirectory(widgets)
endif()

# core

add_executable(demo_argparser demo_argparser.cpp)
target_link_libraries(demo_argparser xci-core)
install(TARGETS demo_argparser EXPORT xcikit DESTINATION examples)

add_executable(demo_dispatch demo_dispatch.cpp)
target_link_libraries(demo_dispatch xci-core)
install(TARGETS demo_dispatch EXPORT xcikit DESTINATION examples)

add_executable(demo_log demo_log.cpp)
target_link_libraries(demo_log xci-core)
install(TARGETS demo_log EXPORT xcikit DESTINATION examples)

add_executable(demo_event demo_event.cpp)
target_link_libraries(demo_event xci-core)
install(TARGETS demo_event EXPORT xcikit DESTINATION examples)

add_executable(demo_filewatch demo_filewatch.cpp)
target_link_libraries(demo_filewatch xci-core)
install(TARGETS demo_filewatch EXPORT xcikit DESTINATION examples)

add_executable(demo_stack demo_stack.cpp)
target_link_libraries(demo_stack xci-core)
install(TARGETS demo_stack EXPORT xcikit DESTINATION examples)

add_executable(demo_termctl demo_termctl.cpp)
target_link_libraries(demo_termctl xci-core)
install(TARGETS demo_termctl EXPORT xcikit DESTINATION examples)

add_executable(demo_vfs demo_vfs.cpp)
target_link_libraries(demo_vfs xci-core)
install(TARGETS demo_vfs EXPORT xcikit DESTINATION examples)

# data

add_executable(demo_reflect reflect/demo_reflect.cpp)
target_link_libraries(demo_reflect xci-data)
install(TARGETS demo_reflect EXPORT xcikit DESTINATION examples)

# script

add_executable(demo_script script/demo_script.cpp)
target_link_libraries(demo_script xci-script)
install(TARGETS demo_script EXPORT xcikit DESTINATION examples)

# text

if (XCI_WITH_VULKAN)
    add_executable(demo_font demo_font.cpp)
    target_link_libraries(demo_font xci-text xci-graphics)
    install(TARGETS demo_font EXPORT xcikit DESTINATION examples)

    add_executable(demo_layout demo_layout.cpp)
    target_link_libraries(demo_layout xci-text xci-graphics)
    install(TARGETS demo_layout EXPORT xcikit DESTINATION examples)
endif()
