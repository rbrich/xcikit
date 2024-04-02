set(DEMO_DEST examples/core)
add_demo(demo_argparser LINK xci-core)

if (NOT EMSCRIPTEN)
    add_demo(demo_dispatch LINK xci-core)
    add_demo(demo_event LINK xci-core)
    add_demo(demo_filewatch LINK xci-core)
endif()

add_demo(demo_log LINK xci-core)
add_demo(demo_chunked_stack LINK xci-core)
add_demo(demo_indexed_map LINK xci-core)
add_demo(demo_termctl LINK xci-core magic_enum::magic_enum)
add_demo(demo_vfs LINK xci-vfs DEPS share)
