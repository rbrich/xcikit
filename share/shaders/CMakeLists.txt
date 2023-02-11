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
# Targets      #
# ------------ #

set(SHADER_SOURCES
    rectangle.vert
    rectangle.frag
    sprite.vert
    sprite.frag
    sprite_r.frag
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
    flat_c.vert
    flat_c.frag
    polygon.vert
    polygon.frag
    )

set(outputs)
set(assets)
file(MAKE_DIRECTORY "${XCIKIT_BINARY_DIR}/share/xcikit/shaders")
foreach(src ${SHADER_SOURCES})
    set(output "${XCIKIT_BINARY_DIR}/share/xcikit/shaders/${src}.spv")
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
        install(FILES "${output}" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/xcikit/shaders)
    endif()
endforeach()

add_custom_target(shaders ALL DEPENDS ${outputs})
set_property(TARGET shaders PROPERTY ASSETS ${assets})
