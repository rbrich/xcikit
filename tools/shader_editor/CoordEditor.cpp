// CoordEditor.cpp created on 2023-03-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "CoordEditor.h"
#include <xci/geometry/Mat3.h>

#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/range/conversion.hpp>

namespace xci::shed {

using namespace xci::core;
using namespace xci::text;
using ranges::views::enumerate;
using ranges::views::transform;
using ranges::to;


CoordEditor::CoordEditor(Theme& theme, Primitives& prim)
    : Widget(theme), m_prim(prim),
      m_rectangle(theme.renderer()),
      m_triangle(theme.renderer()),
      m_circles(theme.renderer())
{}


void CoordEditor::resize(View& view)
{
    Widget::resize(view);
    reconstruct(view);
}


void CoordEditor::update(View& view, State state)
{
    if (m_need_reconstruct) {
        m_need_reconstruct = false;
        reconstruct(view);
    }
    view.refresh();
}


void CoordEditor::draw(View& view)
{
    if (m_is_quad) {
        m_rectangle.draw(view);
    } else {
        m_triangle.draw(view);
    }
    m_circles.draw(view);
}


void CoordEditor::mouse_pos_event(View& view, const MousePosEvent& ev)
{
    // pan
    if (m_dragging && m_active_vertex == ~0u) {
        if (!m_pan_pos) {
            m_pan_pos = ev.pos;
            return;  // initialized
        }
        const auto d = view.fb_to_vp(ev.pos - m_pan_pos);
        if (!d)
            return;  // no change
        m_pan_pos = ev.pos;
        auto& vs = m_is_quad? m_quad_vertices : m_triangle_vertices;
        for (auto& v : vs) {
            v.pos += d;
        }
        m_need_reconstruct = true;
        return;
    }

    // drag vertices
    if (m_dragging && m_active_vertex != ~0u) {
        auto& v = (m_is_quad? m_quad_vertices : m_triangle_vertices)[m_active_vertex];
        const auto np = view.fb_to_vp(ev.pos);
        if (v.pos != np) {
            v.pos = np;
            m_need_reconstruct = true;
        }
        return;
    }

    unsigned idx = 0;
    for (const Point& p : m_is_quad? m_quad_vertices : m_triangle_vertices) {
        const auto mp = view.fb_to_px(ev.pos);
        const auto pp = view.vp_to_px(p.pos);
        if (pp.dist(mp) <= 8_px) {
            if (m_active_vertex != idx) {
                m_active_vertex = idx;
                m_need_reconstruct = true;
            }
            return;
        }
        ++idx;
    }
    if (m_active_vertex != ~0u) {
        m_active_vertex = ~0u;
        m_need_reconstruct = true;
    }
}


bool CoordEditor::mouse_button_event(View& view, const MouseBtnEvent& ev)
{
    if (ev.button != MouseButton::Left)
        return false;

    if (!m_dragging && ev.action == Action::Press) {
        m_dragging = true;
        m_pan_pos = {};
        return true;
    }

    if (m_dragging && ev.action == Action::Release) {
        m_dragging = false;
        return true;
    }

    return false;
}


void CoordEditor::reconstruct(View& view)
{
    struct FbPoint {
        FbPoint(const View& view, const Point& p) : pos(view.vp_to_fb(p.pos)), uv(p.uv) {}
        FramebufferPixels x() const { return pos.x; }
        FramebufferPixels y() const { return pos.y; }
        FramebufferCoords pos;
        Vec2f uv;
    };
    std::vector<FbPoint> points = (m_is_quad? m_quad_vertices : m_triangle_vertices)
             | transform([&view](const Point& p){ return FbPoint(view, p); })
             | to<std::vector>();

    if (m_is_quad) {
        // First point has to be left-top
        if (points[0].x() > points[1].x()) {
            std::swap(points[0].pos.x, points[1].pos.x);
            std::swap(points[0].uv.x, points[1].uv.x);
        }
        if (points[0].y() > points[1].y()) {
            std::swap(points[0].pos.y, points[1].pos.y);
            std::swap(points[0].uv.y, points[1].uv.y);
        }
    } else {
        // Swap points if not CCW
        // https://math.stackexchange.com/a/1324213/1156402
        const float det = Mat3f(points[0].x().value, points[1].x().value, points[2].x().value,
                                points[0].y().value, points[1].y().value, points[2].y().value,
                                1.0f, 1.0f, 1.0f
                                ).determinant();
        if (det > 0.0f) {
            std::swap(points[1], points[2]);
        }
    }

    // a triangle / quad with the shader
    m_prim.clear();
    m_prim.begin_primitive();
    for (const FbPoint& p : points) {
        m_prim.add_vertex(p.pos).uv(p.uv);
        if (m_is_quad) {
            // the other point
            const FbPoint& o = (&p == &points.front())? points.back() : points.front();
            m_prim.add_vertex({p.pos.x, o.pos.y}).uv(p.uv.x, o.uv.y);
        }
    }
    m_prim.end_primitive();
    m_prim.update();


    if (m_is_quad) {
        m_rectangle.clear();
        m_rectangle.add_rectangle({points[0].pos, points[1].pos - points[0].pos},
                                  view.px_to_fb(1_px));
        m_rectangle.update(Color::Transparent(), Color::Grey(), 0, 2);
    } else {
        m_triangle.clear();
        m_triangle.add_triangle(points[0].pos, points[1].pos, points[2].pos, view.px_to_fb(1_px));
        m_triangle.update(Color::Transparent(), Color::Grey(), 0, 2);
    }

    m_circles.clear();
    for (const auto [i, p] : (m_is_quad? m_quad_vertices : m_triangle_vertices) | enumerate) {
        m_circles.add_circle(view.vp_to_fb(p.pos), view.px_to_fb(4_px),
                             i == m_active_vertex ? Color::Maroon() : Color::Black(),
                             i == m_active_vertex ? Color::Yellow() : Color::Grey(),
                             view.px_to_fb(1_px));
    }
    m_circles.update(0, 2);
}


} // namespace xci::shed
