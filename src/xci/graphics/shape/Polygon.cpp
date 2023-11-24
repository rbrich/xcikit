// Polygon.cpp created on 2018-04-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Polygon.h"
#include <xci/math/Vec2.h>


namespace xci::graphics {


void Polygon::add_polygon(FramebufferCoords center, std::span<const FramebufferCoords> vertices,
                          FramebufferPixels outline_thickness)
{
    // Use barycentric coordinates inside each triangle to:
    // * identify the outer edge (it has barycentric Z near 0)
    // * set threshold for the outline (outline drawn where barycentric Z < 1.0)
    // All barycentric coords are multiplied by (distance from center to edge) / outline_thickness.
    assert(vertices.size() >= 2);
    const auto d = dist_point_to_line(center, vertices[0], vertices[1]);
    if (outline_thickness < 0.01f)
        outline_thickness = 0.01f;
    float b1 = (d / outline_thickness).value;
    float b2 = 0.0f;
    m_primitives.begin_primitive();
    m_primitives.add_vertex(center).uvw(0.0f, 0.0f, b1);
    for (const auto vertex : vertices) {
        m_primitives.add_vertex(vertex).uvw(b1, b2, 0.0f);
        std::swap(b1, b2);
    }
    m_primitives.end_primitive();
}


// -----------------------------------------------------------------------------


void ColoredPolygon::add_polygon(FramebufferCoords center, std::span<const FramebufferCoords> vertices,
                                 Color fill_color, Color outline_color, FramebufferPixels outline_thickness)
{
    assert(vertices.size() >= 2);
    const auto d = dist_point_to_line(center, vertices[0], vertices[1]);
    if (outline_thickness < 0.01f)
        outline_thickness = 0.01f;
    float b1 = (d / outline_thickness).value;
    float b2 = 0.0f;
    m_primitives.begin_primitive();
    m_primitives.add_vertex(center).color(fill_color).color(outline_color).uvw(0.0f, 0.0f, b1);
    for (const auto vertex : vertices) {
        m_primitives.add_vertex(vertex).color(fill_color).color(outline_color).uvw(b1, b2, 0.0f);
        std::swap(b1, b2);
    }
    m_primitives.end_primitive();
}


} // namespace xci::graphics
