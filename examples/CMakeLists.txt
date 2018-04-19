add_subdirectory(plugin)

add_executable(demo_log demo_log.cpp)
target_link_libraries(demo_log ${XCI_UTIL_LIBRARY})

add_executable(demo_filewatch demo_filewatch.cpp)
target_link_libraries(demo_filewatch ${XCI_UTIL_LIBRARY})

add_executable(demo_thread demo_thread.cpp)
target_link_libraries(demo_thread ${XCI_TEXT_LIBRARY} pthread)

add_executable(demo_font demo_font.cpp)
target_link_libraries(demo_font ${XCI_TEXT_LIBRARY})

add_executable(demo_fps demo_fps.cpp)
target_link_libraries(demo_fps ${XCI_WIDGETS_LIBRARY})

add_executable(demo_layout demo_layout.cpp)
target_link_libraries(demo_layout ${XCI_TEXT_LIBRARY})

add_executable(demo_coords demo_coords.cpp)
target_link_libraries(demo_coords ${XCI_TEXT_LIBRARY})

add_executable(demo_rectangles demo_rectangles.cpp)
target_link_libraries(demo_rectangles ${XCI_GRAPHICS_LIBRARY})

add_executable(demo_ellipses demo_ellipses.cpp)
target_link_libraries(demo_ellipses ${XCI_GRAPHICS_LIBRARY})

add_executable(demo_rounded_rect demo_rounded_rect.cpp)
target_link_libraries(demo_rounded_rect ${XCI_GRAPHICS_LIBRARY})

add_executable(demo_shapes demo_shapes.cpp)
target_link_libraries(demo_shapes ${XCI_GRAPHICS_LIBRARY})

add_executable(demo_widgets demo_widgets.cpp)
target_link_libraries(demo_widgets ${XCI_WIDGETS_LIBRARY})

if (WITH_OPENGL)
    add_executable(demo_opengl demo_opengl.cpp)
    target_link_libraries(demo_opengl
        ${XCI_TEXT_LIBRARY}
        ${XCI_GRAPHICS_OPENGL_LIBRARY})
endif (WITH_OPENGL)

if (WITH_SFML)
    add_executable(demo_sfml demo_sfml.cpp)
    target_link_libraries(demo_sfml ${XCI_GRAPHICS_SFML_LIBRARY})
endif (WITH_SFML)

if (WITH_PANDA)
    add_executable(demo_panda demo_panda.cpp)
    target_link_libraries(demo_panda
        ${XCI_TEXT_LIBRARY}
        ${XCI_GRAPHICS_PANDA_LIBRARY})
endif (WITH_PANDA)
