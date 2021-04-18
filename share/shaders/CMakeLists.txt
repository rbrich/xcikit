# ------------ #
# Dependencies #
# ------------ #

# shader compiler - can use either glslc or glslangValidator
find_package(GLSLC)

if (NOT GLSLC_FOUND)
    message(STATUS "No GLSL compiler found, won't compile shaders...")
    return()
endif()

# ------------ #
# Functions    #
# ------------ #

function(add_shaders)
    set(options)
    set(one_value_args TARGET)
    set(multi_value_args SOURCES)
    cmake_parse_arguments(ARG "${options}" "${one_value_args}"
        "${multi_value_args}" ${ARGN} )
    if (NOT ARG_TARGET)
        message(SEND_ERROR "Required parameter missing: TARGET")
    endif()
    if (NOT ARG_SOURCES)
        message(SEND_ERROR "Required parameter missing: SOURCES")
    endif()

    set(outputs)
    set(assets)
    foreach(src ${ARG_SOURCES})
        set(output "${CMAKE_CURRENT_BINARY_DIR}/../xcikit/shaders/${src}.spv")
        add_custom_command(
            OUTPUT ${output}
            MAIN_DEPENDENCY "${src}"
            COMMAND ${GLSLC_COMMAND} "${CMAKE_CURRENT_SOURCE_DIR}/${src}" -o ${output}
            COMMENT "Compiling shader ${src}"
            VERBATIM
        )
        list(APPEND outputs ${output})
        list(APPEND assets "shaders/${src}.spv")
        if (XCI_INSTALL_SHARE_DIR)
            install(FILES "${output}"
                DESTINATION share/xcikit/shaders)
        endif()
    endforeach()

    add_custom_target(shaders ALL DEPENDS ${outputs})
    set_property(TARGET shaders PROPERTY ASSETS ${assets})
endfunction()


# ------------ #
# Targets      #
# ------------ #

set(SHADER_SOURCES
    rectangle.vert
    rectangle.frag
    sprite.vert
    sprite.frag
    sprite_c.vert
    sprite_c.frag
    line.vert
    line.frag
    ellipse.vert
    ellipse.frag
    fps.vert
    fps.frag
    cursor.vert
    cursor.frag
    )

add_shaders(TARGET shaders SOURCES ${SHADER_SOURCES})
