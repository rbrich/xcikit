set(DEMO_DEST examples/core)
add_demo(demo_argparser LINK xci-core)

if (NOT EMSCRIPTEN)
    add_demo(demo_dispatch LINK xci-core)
    add_demo(demo_event LINK xci-core)
    add_demo(demo_filewatch LINK xci-core)
endif()

add_demo(demo_log LINK xci-core)
add_demo(demo_chunked_stack LINK xci-core)
add_demo(demo_termctl LINK xci-core magic_enum::magic_enum)
add_demo(demo_vfs LINK xci-core DEPS share)

if (incbin_FOUND)
    set(SHARE_ARCHIVE "${XCIKIT_BINARY_DIR}/${XCI_SHARE_DAR}")
    add_demo(demo_vfs_incbin LINK xci-core incbin::incbin DEPS share
        CDEFS SHARE_ARCHIVE="${SHARE_ARCHIVE}")
endif()
