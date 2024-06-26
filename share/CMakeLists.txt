# ------------ #
# Dependencies #
# ------------ #

if (XCI_INSTALL_SHARE_DAR)
    find_package(Python3 REQUIRED)
else()
    find_package(Python3)
endif()


# ------------ #
# File List    #
# ------------ #

set(ASSETS)

if (XCI_SCRIPT)
    set(ASSETS ${ASSETS} script/std.fire)
endif()

# ------------ #
# Targets      #
# ------------ #

# custom property for association of asset files with targets
# (the list should contain paths relative to share output directory)
define_property(TARGET
    PROPERTY ASSETS
    BRIEF_DOCS "asset files generated by the target"
    FULL_DOCS "the property contains list of paths relative to share output directory")

if (XCI_GRAPHICS)
    add_subdirectory(shaders)
    add_subdirectory(fonts)
endif()

# dummy targets (it's easier to create a target than adding special conditions below)
if (NOT TARGET shaders)
    add_custom_target(shaders ALL)
    set_property(TARGET shaders PROPERTY ASSETS)
endif()
if (NOT TARGET fonts)
    add_custom_target(fonts ALL)
    set_property(TARGET fonts PROPERTY ASSETS)
endif()

# Copy asset files to binary dir
foreach(file ${ASSETS})
    configure_file("${file}" "xcikit/${file}" COPYONLY)
endforeach()

set(output_archives)

# Create data archive: share.dar
if (ASSETS AND Python3_Interpreter_FOUND)
    set(output_dar "share.dar")
    add_custom_command(
        OUTPUT "${output_dar}"
        DEPENDS ${ASSETS} shaders fonts
        COMMAND Python3::Interpreter
            "${PROJECT_SOURCE_DIR}/tools/data_archive/dar_pack.py" -q -c
            "${output_dar}"
            ${ASSETS}
            $<TARGET_PROPERTY:shaders,ASSETS>
            $<TARGET_PROPERTY:fonts,ASSETS>
        COMMAND_EXPAND_LISTS
        WORKING_DIRECTORY xcikit
        COMMENT "Archiving assets (DAR archive)"
        VERBATIM
    )
    if (XCI_INSTALL_SHARE_DAR)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/xcikit/${output_dar}"
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/xcikit)
    endif()
    list(APPEND output_archives ${output_dar})
endif()

# Create data archive: share.zip
if (ASSETS)
    set(output_zip "share.zip")
    add_custom_command(
        OUTPUT "${output_zip}"
        DEPENDS ${ASSETS} shaders fonts
        COMMAND
            ${CMAKE_COMMAND} -E tar cf "${output_zip}" --format=zip
            ${ASSETS}
            $<TARGET_PROPERTY:shaders,ASSETS>
            $<TARGET_PROPERTY:fonts,ASSETS>
        COMMAND_EXPAND_LISTS
        WORKING_DIRECTORY xcikit
        COMMENT "Archiving assets (ZIP archive)"
        VERBATIM
    )
    if (XCI_INSTALL_SHARE_ZIP)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/xcikit/${output_zip}"
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/xcikit)
    endif()
    list(APPEND output_archives ${output_zip})
endif()

add_custom_target(share ALL DEPENDS ${output_archives})

if (XCI_SCRIPT AND XCI_INSTALL_SHARE_DIR)
    install(DIRECTORY script DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/xcikit)
endif()
