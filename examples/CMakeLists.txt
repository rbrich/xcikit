add_subdirectory(plugin)

set(INST_EXAMPLES share/xcikit/examples)

# core

add_executable(demo_log demo_log.cpp)
target_link_libraries(demo_log xci-core)
install(TARGETS demo_log EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_filewatch demo_filewatch.cpp)
target_link_libraries(demo_filewatch xci-core)
install(TARGETS demo_filewatch EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_term demo_term.cpp)
target_link_libraries(demo_term xci-core)
install(TARGETS demo_term EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_vfs demo_vfs.cpp)
target_link_libraries(demo_vfs xci-core)
install(TARGETS demo_vfs EXPORT xcikit DESTINATION ${INST_EXAMPLES})

# graphics

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

add_executable(demo_widgets demo_widgets.cpp)
target_link_libraries(demo_widgets xci-widgets xci-text xci-graphics)
install(TARGETS demo_widgets EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_ui demo_ui.cpp)
target_link_libraries(demo_ui xci-widgets xci-text xci-graphics)
install(TARGETS demo_ui EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_form demo_form.cpp)
target_link_libraries(demo_form xci-widgets xci-text xci-graphics)
install(TARGETS demo_form EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_term_widget demo_term_widget.cpp)
target_link_libraries(demo_term_widget xci-widgets xci-graphics)
install(TARGETS demo_term_widget EXPORT xcikit DESTINATION ${INST_EXAMPLES})

if (XCI_WITH_OPENGL)
    add_executable(demo_glfw demo_glfw.cpp)
    target_link_libraries(demo_glfw
        xci-text
        xci-graphics-opengl)
    target_include_directories(demo_glfw PRIVATE ${CMAKE_SOURCE_DIR}/ext)
    install(TARGETS demo_glfw EXPORT xcikit DESTINATION ${INST_EXAMPLES})
endif ()

if (XCI_WITH_MAGNUM)
    add_subdirectory(in_magnum)
endif ()

if (XCI_WITH_SFML)
    add_executable(demo_sfml demo_sfml.cpp)
    target_link_libraries(demo_sfml
        xci-graphics-sfml)
    install(TARGETS demo_sfml EXPORT xcikit DESTINATION ${INST_EXAMPLES})
endif ()

if (XCI_WITH_PANDA3D)
    add_executable(demo_panda3d demo_panda3d.cpp)
    target_link_libraries(demo_panda3d
        xci-text
        xci-graphics-panda3d)
    install(TARGETS demo_panda3d EXPORT xcikit DESTINATION ${INST_EXAMPLES})
endif ()
