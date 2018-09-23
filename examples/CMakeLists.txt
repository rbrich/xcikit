add_subdirectory(plugin)

set(INST_EXAMPLES share/xcikit/examples)

# core

add_executable(demo_log demo_log.cpp)
target_link_libraries(demo_log ${XCI_CORE_LIBRARY})
install(TARGETS demo_log EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_filewatch demo_filewatch.cpp)
target_link_libraries(demo_filewatch ${XCI_CORE_LIBRARY})
install(TARGETS demo_filewatch EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_term demo_term.cpp)
target_link_libraries(demo_term ${XCI_CORE_LIBRARY})
install(TARGETS demo_term EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_vfs demo_vfs.cpp)
target_link_libraries(demo_vfs ${XCI_CORE_LIBRARY})
install(TARGETS demo_vfs EXPORT xcikit DESTINATION ${INST_EXAMPLES})

# graphics

add_executable(demo_coords demo_coords.cpp)
target_link_libraries(demo_coords ${XCI_TEXT_LIBRARY})
install(TARGETS demo_coords EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_fps demo_fps.cpp)
target_link_libraries(demo_fps ${XCI_WIDGETS_LIBRARY})
install(TARGETS demo_fps EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_shapes demo_shapes.cpp)
target_link_libraries(demo_shapes ${XCI_GRAPHICS_LIBRARY} ${XCI_TEXT_LIBRARY})
install(TARGETS demo_shapes EXPORT xcikit DESTINATION ${INST_EXAMPLES})

# text

add_executable(demo_font demo_font.cpp)
target_link_libraries(demo_font ${XCI_TEXT_LIBRARY})
install(TARGETS demo_font EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_layout demo_layout.cpp)
target_link_libraries(demo_layout ${XCI_TEXT_LIBRARY})
install(TARGETS demo_layout EXPORT xcikit DESTINATION ${INST_EXAMPLES})

# widgets

add_executable(demo_widgets demo_widgets.cpp)
target_link_libraries(demo_widgets ${XCI_WIDGETS_LIBRARY})
install(TARGETS demo_widgets EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_ui demo_ui.cpp)
target_link_libraries(demo_ui ${XCI_WIDGETS_LIBRARY})
install(TARGETS demo_ui EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_form demo_form.cpp)
target_link_libraries(demo_form ${XCI_WIDGETS_LIBRARY})
install(TARGETS demo_form EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_term_widget demo_term_widget.cpp)
target_link_libraries(demo_term_widget ${XCI_WIDGETS_LIBRARY})
install(TARGETS demo_term_widget EXPORT xcikit DESTINATION ${INST_EXAMPLES})

if (XCI_WITH_OPENGL)
    add_executable(demo_opengl demo_opengl.cpp)
    target_link_libraries(demo_opengl
        ${XCI_TEXT_LIBRARY}
        ${XCI_GRAPHICS_OPENGL_LIBRARY})
    target_include_directories(demo_opengl PRIVATE ${CMAKE_SOURCE_DIR}/ext)
    install(TARGETS demo_opengl EXPORT xcikit DESTINATION ${INST_EXAMPLES})
endif ()

if (XCI_WITH_SFML)
    add_executable(demo_sfml demo_sfml.cpp)
    target_link_libraries(demo_sfml ${XCI_GRAPHICS_SFML_LIBRARY})
    install(TARGETS demo_sfml EXPORT xcikit DESTINATION ${INST_EXAMPLES})
endif ()

if (XCI_WITH_PANDA)
    add_executable(demo_panda demo_panda.cpp)
    target_link_libraries(demo_panda
        ${XCI_TEXT_LIBRARY}
        ${XCI_GRAPHICS_PANDA_LIBRARY})
    install(TARGETS demo_panda EXPORT xcikit DESTINATION ${INST_EXAMPLES})
endif ()
