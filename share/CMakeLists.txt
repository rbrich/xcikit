# ------------ #
# Dependencies #
# ------------ #

find_package(Python3)


# ------------ #
# File List    #
# ------------ #

set(ASSETS
    fonts/Enriqueta/Enriqueta-Bold.ttf
    fonts/Enriqueta/Enriqueta-Regular.ttf
    fonts/Enriqueta/OFL.txt
    fonts/Hack/Hack-Bold.ttf
    fonts/Hack/Hack-BoldItalic.ttf
    fonts/Hack/Hack-Italic.ttf
    fonts/Hack/Hack-Regular.ttf
    fonts/MaterialIcons/MaterialIcons-Regular.woff
    fonts/MaterialIcons/LICENSE
    fonts/ShareTechMono/ShareTechMono-Regular.ttf
    fonts/ShareTechMono/OFL.txt
    script/sys.ys
    )


# ------------ #
# Targets      #
# ------------ #

if (XCI_WITH_VULKAN)
    add_subdirectory(shaders)
endif()

# Copy asset files to binary dir
foreach(file ${ASSETS})
    configure_file("${file}" "${file}" COPYONLY)
endforeach()

# Create data archive
if (Python3_Interpreter_FOUND)
    # share.dar
    set(output_dar "${CMAKE_CURRENT_BINARY_DIR}/share.dar")
    add_custom_command(
        OUTPUT "${output_dar}"
        DEPENDS ${ASSETS} shaders
        COMMAND Python3::Interpreter
            "${PROJECT_SOURCE_DIR}/tools/pack_assets.py" -q
            "${output_dar}"
            ${ASSETS} $<TARGET_PROPERTY:shaders,ASSETS>
        COMMAND_EXPAND_LISTS
        COMMENT "Archiving assets (DAR archive)"
        VERBATIM
    )
    install(FILES "${output_dar}" DESTINATION share/xcikit)

    # share.zip
    set(output_zip "${CMAKE_CURRENT_BINARY_DIR}/share.zip")
    add_custom_command(
        OUTPUT "${output_zip}"
        DEPENDS ${ASSETS} shaders
        COMMAND
            ${CMAKE_COMMAND} -E tar cf "${output_zip}" --format=zip
            ${ASSETS} $<TARGET_PROPERTY:shaders,ASSETS>
        COMMAND_EXPAND_LISTS
        COMMENT "Archiving assets (ZIP archive)"
        VERBATIM
    )
    install(FILES "${output_zip}" DESTINATION share/xcikit)

    add_custom_target(archives ALL DEPENDS ${output_dar} ${output_zip})
endif()

# Install data archives and also directories
install(DIRECTORY fonts DESTINATION share/xcikit)
install(DIRECTORY script DESTINATION share/xcikit)
