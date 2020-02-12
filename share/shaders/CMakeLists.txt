# ------------ #
# Dependencies #
# ------------ #

find_program(GLSLC glslc)

if (NOT GLSLC)
    return()
endif()

# ------------ #
# Functions    #
# ------------ #

function(add_shaders)
    cmake_parse_arguments(ARG "" "TARGET" "SOURCES" ${ARGN})
    if (NOT ARG_TARGET)
        message(SEND_ERROR "Required parameter missing: TARGET")
    endif()
    if (NOT ARG_SOURCES)
        message(SEND_ERROR "Required parameter missing: SOURCES")
    endif()

    set(outputs)
    foreach(src ${ARG_SOURCES})
        add_custom_command(
            OUTPUT "${src}.spv"
            MAIN_DEPENDENCY "${src}"
            COMMAND ${GLSLC} "${CMAKE_CURRENT_SOURCE_DIR}/${src}" -o "${src}.spv"
            COMMENT "Compiling shader ${src}"
            VERBATIM
        )
        list(APPEND outputs "${src}.spv")
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${src}.spv" DESTINATION share/xcikit/shaders)
    endforeach()

    add_custom_target(shaders ALL DEPENDS ${outputs})
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