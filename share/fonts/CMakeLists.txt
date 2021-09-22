# ------------ #
# Fetch Fonts  #
# -------------#

# Hack font - download and extract in SOURCE dir
message(STATUS "Assets: Hack font")
set(HACK_VERSION "v3.003")
set(HACK_ARCHIVE "Hack-${HACK_VERSION}-ttf.tar.xz")
file(DOWNLOAD
    https://github.com/source-foundry/Hack/releases/download/${HACK_VERSION}/${HACK_ARCHIVE}
    ${CMAKE_CURRENT_SOURCE_DIR}/${HACK_ARCHIVE}
    EXPECTED_HASH SHA1=7ab9e67f718a1a7f3a7bb4426a4a33b949a911f3
    SHOW_PROGRESS)
if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Hack/Hack-Regular.ttf)
    file(ARCHIVE_EXTRACT INPUT ${CMAKE_CURRENT_SOURCE_DIR}/${HACK_ARCHIVE}
        DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/Hack
        VERBOSE)
endif()

set(FONTS
    Enriqueta/Enriqueta-Bold.ttf
    Enriqueta/Enriqueta-Regular.ttf
    Hack/Hack-Bold.ttf
    Hack/Hack-BoldItalic.ttf
    Hack/Hack-Italic.ttf
    Hack/Hack-Regular.ttf
    MaterialIcons/MaterialIcons-Regular.woff
    ShareTechMono/ShareTechMono-Regular.ttf
    )

set(outputs)
set(assets)
foreach(src ${FONTS})
    set(output "${XCIKIT_BINARY_DIR}/share/xcikit/fonts/${src}")
    configure_file(${src} ${output} COPYONLY)
    list(APPEND outputs ${output})
    list(APPEND assets "fonts/${src}")
endforeach()

add_custom_target(fonts ALL DEPENDS ${outputs})
set_property(TARGET fonts PROPERTY ASSETS ${assets})

if (XCI_INSTALL_SHARE_DIR)
    install(DIRECTORY . DESTINATION share/xcikit)
endif()
