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
    cursor.vert
    cursor.frag
    ellipse.vert
    ellipse.frag
    ellipse_c.vert
    ellipse_c.frag
    flat.vert
    flat.frag
    flat_c.vert
    flat_c.frag
    fps.vert
    fps.frag
    line.vert
    line.frag
    line_c.vert
    line_c.frag
    phong.vert
    phong.frag
    pick.vert
    pick.frag
    polygon.vert
    polygon.frag
    polygon_c.vert
    polygon_c.frag
    rectangle.vert
    rectangle.frag
    rectangle_c.vert
    rectangle_c.frag
    rounded_rectangle.vert
    rounded_rectangle.frag
    rounded_rectangle_c.vert
    rounded_rectangle_c.frag
    sprite.vert
    sprite.frag
    sprite_c.vert
    sprite_c.frag
    sprite_r.frag
    triangle.frag
    triangle_c.frag
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
