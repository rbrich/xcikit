// Triangle.cpp created on 2023-02-25 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Triangle.h"
#include <xci/core/geometry.h>


namespace xci::graphics {


void Triangle::add_triangle(FramebufferCoords v1, FramebufferCoords v2, FramebufferCoords v3,
                            FramebufferPixels outline_thickness)
{
    // Use barycentric coordinates inside each triangle to:
    // * identify the outer edge (it has minimum of barycentric axes near 0)
    // * set threshold for the outline (outline drawn where barycentric minimum < 1.0)
    // All barycentric coords are multiplied by (distance from center to edge) / outline_thickness.
    if (outline_thickness < 0.01)
        outline_thickness = 0.01;
    const float b1 = (xci::core::dist_point_to_line(v1, v2, v3) / outline_thickness).value;
    const float b2 = (xci::core::dist_point_to_line(v2, v3, v1) / outline_thickness).value;
    const float b3 = (xci::core::dist_point_to_line(v3, v1, v2) / outline_thickness).value;
    m_primitives.begin_primitive();
    m_primitives.add_vertex(v1).uvw(b1, 0.0f, 0.0f);
    m_primitives.add_vertex(v2).uvw(0.0f, b2, 0.0f);
    m_primitives.add_vertex(v3).uvw(0.0f, 0.0f, b3);
    m_primitives.end_primitive();
}


// -----------------------------------------------------------------------------


void ColoredTriangle::add_triangle(FramebufferCoords v1, FramebufferCoords v2, FramebufferCoords v3,
                                   Color fill_color, Color outline_color, FramebufferPixels outline_thickness)
{
    if (outline_thickness < 0.01)
        outline_thickness = 0.01;
    const float b1 = (xci::core::dist_point_to_line(v1, v2, v3) / outline_thickness).value;
    const float b2 = (xci::core::dist_point_to_line(v2, v3, v1) / outline_thickness).value;
    const float b3 = (xci::core::dist_point_to_line(v3, v1, v2) / outline_thickness).value;
    m_primitives.begin_primitive();
    m_primitives.add_vertex(v1).color(fill_color).color(outline_color).uvw(b1, 0.0f, 0.0f);
    m_primitives.add_vertex(v2).color(fill_color).color(outline_color).uvw(0.0f, b2, 0.0f);
    m_primitives.add_vertex(v3).color(fill_color).color(outline_color).uvw(0.0f, 0.0f, b3);
    m_primitives.end_primitive();
}


} // namespace xci::graphics
