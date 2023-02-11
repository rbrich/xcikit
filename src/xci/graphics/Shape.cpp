// Shape.cpp created on 2018-04-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Shape.h"
#include "Renderer.h"
#include <xci/core/geometry.h>
#include <cmath>
#include <cstdlib>


namespace xci::graphics {


Shape::Shape(Renderer& renderer, Color fill_color, Color outline_color)
        : m_fill_color(fill_color), m_outline_color(outline_color),
          m_lines(renderer, VertexFormat::V2t2, PrimitiveType::TriFans),
          m_rectangles(renderer, VertexFormat::V2c4t22, PrimitiveType::TriFans),
          m_ellipses(renderer, VertexFormat::V2t22, PrimitiveType::TriFans),
          m_polygons(renderer, VertexFormat::V2t3, PrimitiveType::TriFans),
          m_line_shader(renderer.get_shader(ShaderId::Line)),
          m_rectangle_shader(renderer.get_shader(ShaderId::Rectangle)),
          m_ellipse_shader(renderer.get_shader(ShaderId::Ellipse)),
          m_polygon_shader(renderer.get_shader(ShaderId::Polygon))
{}


void Shape::add_line(FramebufferCoords a, FramebufferCoords b, FramebufferPixels thickness)
{
    add_line_slice(
            { std::min(a.x, b.x), std::min(a.y, b.y),
              std::abs(a.x.value - b.x.value), std::abs(a.y.value - b.y.value) },
            a, b, thickness);
}


void Shape::add_line_slice(const FramebufferRect& slice,
                           FramebufferCoords a, FramebufferCoords b,
                           FramebufferPixels thickness)
{
    auto dir = (b-a).norm();
    auto rotate = [dir](float x, float y) -> Vec2f {
        float xnew = x * dir.x.value + y * dir.y.value;
        float ynew = -x * dir.y.value + y * dir.x.value;
        return {xnew, ynew};
    };

    auto x1 = slice.x;
    auto y1 = slice.y;
    auto x2 = slice.x + slice.w;
    auto y2 = slice.y + slice.h;
    float ax = ((slice.x - a.x) / thickness).value;
    float ay = ((slice.y - a.y) / thickness).value;
    float bx = ((x2 - a.x) / thickness).value;
    float by = ((y2 - a.y) / thickness).value;
    auto t1 = rotate(ax, ay);
    auto t2 = rotate(ax, by);
    auto t3 = rotate(bx, by);
    auto t4 = rotate(bx, ay);
    m_lines.begin_primitive();
    m_lines.add_vertex({x1, y1}, t1.x, t1.y);
    m_lines.add_vertex({x1, y2}, t2.x, t2.y);
    m_lines.add_vertex({x2, y2}, t3.x, t3.y);
    m_lines.add_vertex({x2, y1}, t4.x, t4.y);
    m_lines.end_primitive();
}


void Shape::add_rectangle(const FramebufferRect& rect, FramebufferPixels outline_thickness)
{
    auto x1 = rect.x;
    auto y1 = rect.y;
    auto x2 = rect.x + rect.w;
    auto y2 = rect.y + rect.h;
    float tx = 2.0f * outline_thickness.value / rect.w.value;
    float ty = 2.0f * outline_thickness.value / rect.h.value;
    float ix = 1.0f + tx / (1.0f - tx);
    float iy = 1.0f + ty / (1.0f - ty);

    m_rectangles.begin_primitive();
    m_rectangles.add_vertex({x1, y1}, m_fill_color, -ix, -iy, -1.0f, -1.0f);
    m_rectangles.add_vertex({x1, y2}, m_fill_color, -ix, +iy, -1.0f, +1.0f);
    m_rectangles.add_vertex({x2, y2}, m_fill_color, +ix, +iy, +1.0f, +1.0f);
    m_rectangles.add_vertex({x2, y1}, m_fill_color, +ix, -iy, +1.0f, -1.0f);
    m_rectangles.end_primitive();
}


void Shape::add_rectangle_slice(const FramebufferRect& slice, const FramebufferRect& rect,
                                FramebufferPixels outline_thickness)
{
    auto x1 = slice.x;
    auto y1 = slice.y;
    auto x2 = slice.x + slice.w;
    auto y2 = slice.y + slice.h;
    auto rcx = rect.x + rect.w/2;
    auto rcy = rect.y + rect.h/2;
    float ax = (2.0f * (slice.x - rcx) / rect.w).value;
    float ay = (2.0f * (slice.y - rcy) / rect.h).value;
    float bx = (2.0f * (x2 - rcx) / rect.w).value;
    float by = (2.0f * (y2 - rcy) / rect.h).value;
    float tx = (2.0f * outline_thickness / rect.w).value;
    float ty = (2.0f * outline_thickness / rect.h).value;
    float cx = ax * (1.0f + tx / (1-tx));
    float cy = ay * (1.0f + ty / (1-ty));
    float dx = bx * (1.0f + tx / (1-tx));
    float dy = by * (1.0f + ty / (1-ty));
    m_rectangles.begin_primitive();
    m_rectangles.add_vertex({x1, y1}, m_fill_color, cx, cy, ax, ay);
    m_rectangles.add_vertex({x1, y2}, m_fill_color, cx, dy, ax, by);
    m_rectangles.add_vertex({x2, y2}, m_fill_color, dx, dy, bx, by);
    m_rectangles.add_vertex({x2, y1}, m_fill_color, dx, cy, bx, ay);
    m_rectangles.end_primitive();
}


void Shape::add_ellipse(const FramebufferRect& rect, FramebufferPixels outline_thickness)
{
    auto x1 = rect.x;
    auto y1 = rect.y;
    auto x2 = rect.x + rect.w;
    auto y2 = rect.y + rect.h;
    float tx = (2.0f * outline_thickness / rect.w).value;
    float ty = (2.0f * outline_thickness / rect.h).value;
    float ix = 1.0f + tx / (1-tx);
    float iy = 1.0f + ty / (1-ty);
    m_ellipses.begin_primitive();
    m_ellipses.add_vertex({x1, y1}, -ix, -iy, -1.0f, -1.0f);
    m_ellipses.add_vertex({x1, y2}, -ix, +iy, -1.0f, +1.0f);
    m_ellipses.add_vertex({x2, y2}, +ix, +iy, +1.0f, +1.0f);
    m_ellipses.add_vertex({x2, y1}, +ix, -iy, +1.0f, -1.0f);
    m_ellipses.end_primitive();
}


void Shape::add_ellipse_slice(const FramebufferRect& slice, const FramebufferRect& ellipse,
                              FramebufferPixels outline_thickness)
{
    auto x1 = slice.x;
    auto y1 = slice.y;
    auto x2 = slice.x + slice.w;
    auto y2 = slice.y + slice.h;
    float ax = (2.0f * (slice.x - ellipse.x-ellipse.w/2) / ellipse.w).value;
    float ay = (2.0f * (slice.y - ellipse.y-ellipse.h/2) / ellipse.h).value;
    float bx = (2.0f * (slice.x+slice.w - ellipse.x-ellipse.w/2) / ellipse.w).value;
    float by = (2.0f * (slice.y+slice.h - ellipse.y-ellipse.h/2) / ellipse.h).value;
    float tx = (2.0f * outline_thickness / ellipse.w).value;
    float ty = (2.0f * outline_thickness / ellipse.h).value;
    float cx = ax * (1.0f + tx / (1-tx));
    float cy = ay * (1.0f + ty / (1-ty));
    float dx = bx * (1.0f + tx / (1-tx));
    float dy = by * (1.0f + ty / (1-ty));
    m_ellipses.begin_primitive();
    m_ellipses.add_vertex({x1, y1}, cx, cy, ax, ay);
    m_ellipses.add_vertex({x1, y2}, cx, dy, ax, by);
    m_ellipses.add_vertex({x2, y2}, dx, dy, bx, by);
    m_ellipses.add_vertex({x2, y1}, dx, cy, bx, ay);
    m_ellipses.end_primitive();
}


void Shape::add_circle(FramebufferCoords center, float radius, FramebufferPixels outline_thickness)
{
    add_ellipse({center.x - radius, center.y - radius, 2 * radius, 2 * radius},
                outline_thickness);
}


void Shape::add_rounded_rectangle(const FramebufferRect& rect, FramebufferPixels radius,
                                  FramebufferPixels outline_thickness)
{
    // The shape is composed of 7-slice pattern:
    // corner ellipse slices and center rectangle slices
    auto x = rect.x;
    auto y = rect.y;
    auto w = rect.w;
    auto h = rect.h;
    auto r = std::max(radius, outline_thickness * 1.1f);
    auto rr = 2 * r;
    add_ellipse_slice({x,     y,     r, r}, {x,      y,      rr, rr}, outline_thickness);
    add_ellipse_slice({x+w-r, y,     r, r}, {x+w-rr, y,      rr, rr}, outline_thickness);
    add_ellipse_slice({x,     y+h-r, r, r}, {x,      y+h-rr, rr, rr}, outline_thickness);
    add_ellipse_slice({x+w-r, y+h-r, r, r}, {x+w-rr, y+h-rr, rr, rr}, outline_thickness);
    add_rectangle_slice({x+r, y,     w-rr, r}, rect, outline_thickness);
    add_rectangle_slice({x+r, y+h-r, w-rr, r}, rect, outline_thickness);
    add_rectangle_slice({x,   y+r,   w, h-rr}, rect, outline_thickness);
}


void Shape::add_polygon(FramebufferCoords center, std::span<const FramebufferCoords> vertices,
                        FramebufferPixels outline_thickness)
{
    // Use barycentric coordinates inside each triangle to:
    // * identify the outer edge (it has barycentric Z near 0)
    // * set threshold for the outline (outline drawn where barycentric Z < 1.0)
    // All barycentric coords are multiplied by (distance from center to edge) / outline_thickness.
    assert(vertices.size() >= 2);
    const auto d = xci::core::dist_point_to_line(center, vertices[0], vertices[1]);
    float b1 = (d / outline_thickness).value;
    float b2 = 0.0f;
    m_polygons.begin_primitive();
    m_polygons.add_vertex(center, 0.0f, 0.0f, b1);
    for (const auto vertex : vertices) {
        m_polygons.add_vertex(vertex, b1, b2, 0.0f);
        std::swap(b1, b2);
    }
    m_polygons.end_primitive();
}


void Shape::clear()
{
    m_lines.clear();
    m_rectangles.clear();
    m_ellipses.clear();
    m_polygons.clear();
}


void Shape::update()
{
    // lines
    if (!m_lines.empty()) {
        m_lines.clear_uniforms();
        m_lines.add_uniform(1, m_fill_color, m_outline_color);
        m_lines.add_uniform(2, m_softness, m_antialiasing);
        m_lines.set_shader(m_line_shader);
        m_lines.set_blend(BlendFunc::AlphaBlend);
        m_lines.update();
    }

    // rectangles
    if (!m_rectangles.empty()) {
        m_rectangles.clear_uniforms();
        m_rectangles.add_uniform(1, m_outline_color);
        m_rectangles.add_uniform(2, m_softness, m_antialiasing);
        m_rectangles.set_shader(m_rectangle_shader);
        m_rectangles.set_blend(BlendFunc::AlphaBlend);
        m_rectangles.update();
    }

    // ellipses
    if (!m_ellipses.empty()) {
        m_ellipses.clear_uniforms();
        m_ellipses.add_uniform(1, m_fill_color, m_outline_color);
        m_ellipses.add_uniform(2, m_softness, m_antialiasing);
        m_ellipses.set_shader(m_ellipse_shader);
        m_ellipses.set_blend(BlendFunc::AlphaBlend);
        m_ellipses.update();
    }

    // polygons
    if (!m_polygons.empty()) {
        m_polygons.clear_uniforms();
        m_polygons.add_uniform(1, m_fill_color, m_outline_color);
        m_polygons.add_uniform(2, m_softness, m_antialiasing);
        m_polygons.set_shader(m_polygon_shader);
        m_polygons.set_blend(BlendFunc::AlphaBlend);
        m_polygons.update();
    }
}


void Shape::draw(View& view, VariCoords pos)
{
    // lines
    if (!m_lines.empty())
        m_lines.draw(view, pos);

    // rectangles
    if (!m_rectangles.empty())
        m_rectangles.draw(view, pos);

    // ellipses
    if (!m_ellipses.empty())
        m_ellipses.draw(view, pos);

    // polygons
    if (!m_polygons.empty())
        m_polygons.draw(view, pos);
}


// -----------------------------------------------------------------------------


ShapeBuilder& ShapeBuilder::add_line_slice(const VariRect& slice, VariCoords a, VariCoords b, VariUnits thickness)
{
    m_shape.add_line_slice(m_view.to_fb(slice), m_view.to_fb(a), m_view.to_fb(b), m_view.to_fb(thickness));
    return *this;
}


ShapeBuilder& ShapeBuilder::add_rectangle(const VariRect& rect, VariUnits outline_thickness)
{
    m_shape.add_rectangle(m_view.to_fb(rect), m_view.to_fb(outline_thickness));
    return *this;
}


ShapeBuilder& ShapeBuilder::add_rectangle_slice(const VariRect& slice, const VariRect& rect, VariUnits outline_thickness)
{
    m_shape.add_rectangle_slice(m_view.to_fb(slice), m_view.to_fb(rect), m_view.to_fb(outline_thickness));
    return *this;
}


ShapeBuilder& ShapeBuilder::add_ellipse(const VariRect& rect, VariUnits outline_thickness)
{
    m_shape.add_ellipse(m_view.to_fb(rect), m_view.to_fb(outline_thickness));
    return *this;
}


ShapeBuilder& ShapeBuilder::add_ellipse_slice(const VariRect& slice, const VariRect& ellipse, VariUnits outline_thickness)
{
    m_shape.add_ellipse_slice(m_view.to_fb(slice), m_view.to_fb(ellipse), m_view.to_fb(outline_thickness));
    return *this;
}


ShapeBuilder& ShapeBuilder::add_rounded_rectangle(const VariRect& rect, VariUnits radius, VariUnits outline_thickness)
{
    m_shape.add_rounded_rectangle(m_view.to_fb(rect), m_view.to_fb(radius), m_view.to_fb(outline_thickness));
    return *this;
}


} // namespace xci::graphics
