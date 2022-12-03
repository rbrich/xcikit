# ------------ #
# Fetch Fonts  #
# -------------#

message(STATUS "Locking directory: ${CMAKE_CURRENT_SOURCE_DIR}")
file(LOCK ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY
    GUARD FILE)

# Roboto Mono font - download and extract in SOURCE dir
message(STATUS "Assets: Roboto Mono font")
set(ROBOTO_MONO_REVISION "8f651634e746da6df6c2c0be73255721d24f2372")
file(DOWNLOAD
    https://github.com/googlefonts/RobotoMono/raw/${ROBOTO_MONO_REVISION}/fonts/variable/RobotoMono%5Bwght%5D.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/RobotoMono/RobotoMono_wght.ttf
    EXPECTED_HASH SHA1=1f6afd30d77cb011d0747c81e0e62ff0b29b3fa7
    SHOW_PROGRESS)
file(DOWNLOAD
    https://github.com/googlefonts/RobotoMono/raw/${ROBOTO_MONO_REVISION}/fonts/variable/RobotoMono-Italic%5Bwght%5D.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/RobotoMono/RobotoMono-Italic_wght.ttf
    EXPECTED_HASH SHA1=ab937906afc3b6deff56e02a72ea7f8e3f61006e
    SHOW_PROGRESS)

# Lora font - download and extract in SOURCE dir
message(STATUS "Assets: Lora font")
set(LORA_VERSION "v3.003")
set(LORA_ARCHIVE "Lora-${LORA_VERSION}.zip")
file(DOWNLOAD
    https://github.com/cyrealtype/Lora-Cyrillic/releases/download/${LORA_VERSION}/${LORA_ARCHIVE}
    ${CMAKE_CURRENT_SOURCE_DIR}/${LORA_ARCHIVE}
    EXPECTED_HASH SHA1=674844b12428ddbbcfa595ca0efecce2c8ff291d
    SHOW_PROGRESS)
if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Lora/Lora[wght].ttf)
    file(ARCHIVE_EXTRACT INPUT ${CMAKE_CURRENT_SOURCE_DIR}/${LORA_ARCHIVE}
        PATTERNS variable/*.ttf
        DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
        VERBOSE)
    file(RENAME ${CMAKE_CURRENT_SOURCE_DIR}/variable ${CMAKE_CURRENT_SOURCE_DIR}/Lora)
endif()

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

# Noto emoji font - download in SOURCE dir
message(STATUS "Assets: Noto emoji font")
set(NOTO_REVISION "c05b4b47c8250ccd232780cae46d66a8179e77ab")
file(DOWNLOAD
    https://github.com/googlefonts/noto-emoji/raw/${NOTO_REVISION}/fonts/NotoColorEmoji.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Noto/NotoColorEmoji.ttf
    EXPECTED_HASH SHA1=82e8a1244869f8c6a8e1adf5f4ad665456366845
    SHOW_PROGRESS)

set(FONTS
    Enriqueta/Enriqueta-Bold.ttf
    Enriqueta/Enriqueta-Regular.ttf
    Hack/Hack-Bold.ttf
    Hack/Hack-BoldItalic.ttf
    Hack/Hack-Italic.ttf
    Hack/Hack-Regular.ttf
    Lora/Lora[wght].ttf
    Lora/Lora-Italic[wght].ttf
    Noto/NotoColorEmoji.ttf
    MaterialIcons/MaterialIcons-Regular.woff
    RobotoMono/RobotoMono_wght.ttf
    RobotoMono/RobotoMono-Italic_wght.ttf
    ShareTechMono/ShareTechMono-Regular.ttf
    )

set(outputs)
set(assets)
foreach(src ${FONTS})
    set(output "${XCIKIT_BINARY_DIR}/share/xcikit/fonts/${src}")
    configure_file(${src} ${output} COPYONLY)
    list(APPEND outputs ${output})
    list(APPEND assets "fonts/${src}")
    if (XCI_INSTALL_SHARE_DIR)
        get_filename_component(dirname ${src} DIRECTORY)
        install(FILES "${output}" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/xcikit/fonts/${dirname})
    endif()
endforeach()

add_custom_target(fonts ALL DEPENDS ${outputs})
set_property(TARGET fonts PROPERTY ASSETS ${assets})
