# ------------ #
# Dependencies #
# ------------ #

find_path(INCBIN_INCLUDE_DIR incbin.h)

# ------------ #
# Examples     #
# ------------ #

# xci-core

add_subdirectory(core)
add_subdirectory(plugin)

# xci-data

if (XCI_DATA)
    add_executable(demo_serialize data/demo_serialize.cpp)
    target_link_libraries(demo_serialize xci-data)
    install(TARGETS demo_serialize EXPORT xcikit DESTINATION examples)
endif()

# xci-script

if (XCI_SCRIPT)
    add_executable(demo_script script/demo_script.cpp)
    target_link_libraries(demo_script xci-script)
    install(TARGETS demo_script EXPORT xcikit DESTINATION examples)
endif()

# xci-graphics

if (XCI_GRAPHICS)
    add_subdirectory(graphics)
endif()

# xci-text

if (XCI_TEXT)
    add_executable(demo_font demo_font.cpp)
    target_link_libraries(demo_font xci-text xci-graphics)
    install(TARGETS demo_font EXPORT xcikit DESTINATION examples)

    add_executable(demo_layout demo_layout.cpp)
    target_link_libraries(demo_layout xci-text xci-graphics)
    install(TARGETS demo_layout EXPORT xcikit DESTINATION examples)
endif()

# xci-widgets

if (XCI_WIDGETS)
    add_subdirectory(widgets)
endif()
