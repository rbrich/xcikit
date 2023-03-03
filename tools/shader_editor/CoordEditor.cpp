// CoordEditor.cpp created on 2023-03-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "CoordEditor.h"

#include <range/v3/view/enumerate.hpp>

namespace xci::shed {

using namespace xci::core;
using namespace xci::text;
using ranges::views::enumerate;


CoordEditor::CoordEditor(Theme& theme, Primitives& prim)
    : Widget(theme), m_prim(prim),
      m_poly(theme.renderer()), m_triangle(theme.renderer()), m_circles(theme.renderer())
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
    if (m_is_triangle) {
        m_triangle.draw(view);
    } else {
        m_poly.draw(view);
    }
    m_circles.draw(view);
}


void CoordEditor::mouse_pos_event(View& view, const MousePosEvent& ev)
{
    if (m_dragging && m_active_vertex != ~0u) {
        auto& v = (m_is_triangle? m_triangle_vertices : m_quad_vertices)[m_active_vertex];
        const auto np = view.fb_to_vp(ev.pos);
        if (v.xy != np) {
            v.xy = np;
            m_need_reconstruct = true;
        }
        return;
    }

    unsigned idx = 0;
    for (const Point& p : m_is_triangle? m_triangle_vertices : m_quad_vertices) {
        const auto mp = view.fb_to_px(ev.pos);
        const auto pp = view.vp_to_px(p.xy);
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
    // a triangle / quad with the shader
    m_prim.clear();
    m_prim.begin_primitive();
    for (const Point& p : m_is_triangle? m_triangle_vertices : m_quad_vertices) {
        m_prim.add_vertex(view.vp_to_fb(p.xy)).uv(p.uv);
    }
    m_prim.end_primitive();
    m_prim.update();

    std::vector<FramebufferCoords> points;
    if (m_is_triangle) {
        points = {
            view.vp_to_fb(m_triangle_vertices[0].xy),
            view.vp_to_fb(m_triangle_vertices[1].xy),
            view.vp_to_fb(m_triangle_vertices[2].xy)};
    } else {
        points = {
            view.vp_to_fb(m_quad_vertices[0].xy),
            view.vp_to_fb(m_quad_vertices[1].xy),
            view.vp_to_fb(m_quad_vertices[2].xy),
            view.vp_to_fb(m_quad_vertices[3].xy)};
    }

    if (m_is_triangle) {
        m_triangle.clear();
        m_triangle.add_triangle(points[0], points[1], points[2], view.px_to_fb(1_px));
        m_triangle.update(Color::Transparent(), Color::Grey(), 0, 2);
    } else {
        m_poly.clear();
        points.push_back(points.front());
        m_poly.add_polygon(view.vp_to_fb({0_vp, 0_vp}), points, view.px_to_fb(1_px));
        m_poly.update(Color::Transparent(), Color::Grey(), 0, 2);
    }

    m_circles.clear();
    for (const auto [i, p] : points | enumerate) {
        m_circles.add_circle(p, view.px_to_fb(4_px),
                             i == m_active_vertex ? Color::Maroon() : Color::Black(),
                             i == m_active_vertex ? Color::Yellow() : Color::Grey(),
                             view.px_to_fb(1_px));
    }
    m_circles.update(0, 2);
}


} // namespace xci::shed
