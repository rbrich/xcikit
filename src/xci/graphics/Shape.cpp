// Shape.cpp created on 2018-04-04, part of XCI toolkit
// Copyright 2018, 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Shape.h"
#include "Renderer.h"
#include <xci/config.h>
#include <xci/core/log.h>

#ifdef XCI_EMBED_SHADERS
#define INCBIN_PREFIX g_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include <incbin.h>
INCBIN(line_vert, XCI_SHARE_DIR "/shaders/line.vert");
INCBIN(line_frag, XCI_SHARE_DIR "/shaders/line.frag");
INCBIN(rectangle_vert, XCI_SHARE_DIR "/shaders/rectangle.vert");
INCBIN(rectangle_frag, XCI_SHARE_DIR "/shaders/rectangle.frag");
INCBIN(ellipse_vert, XCI_SHARE_DIR "/shaders/ellipse.vert");
INCBIN(ellipse_frag, XCI_SHARE_DIR "/shaders/ellipse.frag");
#endif

namespace xci::graphics {

using namespace xci::core::log;


Shape::Shape(Renderer& renderer, const Color& fill_color, const Color& outline_color)
        : m_renderer(renderer),
          m_fill_color(fill_color), m_outline_color(outline_color),
          m_lines(renderer.create_primitives(VertexFormat::V2t2, PrimitiveType::TriFans)),
          m_rectangles(renderer.create_primitives(VertexFormat::V2c4t22, PrimitiveType::TriFans)),
          m_ellipses(renderer.create_primitives(VertexFormat::V2t22, PrimitiveType::TriFans))
{}


void Shape::add_line_slice(const ViewportRect& slice,
                           const ViewportCoords& a, const ViewportCoords& b,
                           ViewportUnits thickness)
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
    m_lines->begin_primitive();
    m_lines->add_vertex({x1, y1}, t1.x, t1.y);
    m_lines->add_vertex({x1, y2}, t2.x, t2.y);
    m_lines->add_vertex({x2, y2}, t3.x, t3.y);
    m_lines->add_vertex({x2, y1}, t4.x, t4.y);
    m_lines->end_primitive();
}


void Shape::add_rectangle(const ViewportRect& rect, ViewportUnits outline_thickness)
{
    auto x1 = rect.x;
    auto y1 = rect.y;
    auto x2 = rect.x + rect.w;
    auto y2 = rect.y + rect.h;
    float tx = 2.0f * outline_thickness.value / rect.w.value;
    float ty = 2.0f * outline_thickness.value / rect.h.value;
    float ix = 1.0f + tx / (1.0f - tx);
    float iy = 1.0f + ty / (1.0f - ty);

    m_rectangles->begin_primitive();
    m_rectangles->add_vertex({x1, y1}, m_fill_color, -ix, -iy, -1.0f, -1.0f);
    m_rectangles->add_vertex({x1, y2}, m_fill_color, -ix, +iy, -1.0f, +1.0f);
    m_rectangles->add_vertex({x2, y2}, m_fill_color, +ix, +iy, +1.0f, +1.0f);
    m_rectangles->add_vertex({x2, y1}, m_fill_color, +ix, -iy, +1.0f, -1.0f);
    m_rectangles->end_primitive();
}


void Shape::add_rectangle_slice(const ViewportRect& slice, const ViewportRect& rect,
                                ViewportUnits outline_thickness)
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
    m_rectangles->begin_primitive();
    m_rectangles->add_vertex({x1, y1}, m_fill_color, cx, cy, ax, ay);
    m_rectangles->add_vertex({x1, y2}, m_fill_color, cx, dy, ax, by);
    m_rectangles->add_vertex({x2, y2}, m_fill_color, dx, dy, bx, by);
    m_rectangles->add_vertex({x2, y1}, m_fill_color, dx, cy, bx, ay);
    m_rectangles->end_primitive();
}


void Shape::add_ellipse(const ViewportRect& rect, ViewportUnits outline_thickness)
{
    auto x1 = rect.x;
    auto y1 = rect.y;
    auto x2 = rect.x + rect.w;
    auto y2 = rect.y + rect.h;
    float tx = (2.0f * outline_thickness / rect.w).value;
    float ty = (2.0f * outline_thickness / rect.h).value;
    float ix = 1.0f + tx / (1-tx);
    float iy = 1.0f + ty / (1-ty);
    m_ellipses->begin_primitive();
    m_ellipses->add_vertex({x1, y1}, -ix, -iy, -1.0f, -1.0f);
    m_ellipses->add_vertex({x1, y2}, -ix, +iy, -1.0f, +1.0f);
    m_ellipses->add_vertex({x2, y2}, +ix, +iy, +1.0f, +1.0f);
    m_ellipses->add_vertex({x2, y1}, +ix, -iy, +1.0f, -1.0f);
    m_ellipses->end_primitive();
}


void Shape::add_ellipse_slice(const ViewportRect& slice, const ViewportRect& ellipse,
                              ViewportUnits outline_thickness)
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
    m_ellipses->begin_primitive();
    m_ellipses->add_vertex({x1, y1}, cx, cy, ax, ay);
    m_ellipses->add_vertex({x1, y2}, cx, dy, ax, by);
    m_ellipses->add_vertex({x2, y2}, dx, dy, bx, by);
    m_ellipses->add_vertex({x2, y1}, dx, cy, bx, ay);
    m_ellipses->end_primitive();
}


void
Shape::add_rounded_rectangle(const ViewportRect& rect, ViewportUnits radius,
                             ViewportUnits outline_thickness)
{
    // the shape is composed from 7-slice pattern:
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

void Shape::clear()
{
    m_lines->clear();
    m_rectangles->clear();
    m_ellipses->clear();
}


void Shape::reserve(size_t lines, size_t rectangles, size_t ellipses)
{
    m_lines->reserve(lines, 4 * lines);
    m_rectangles->reserve(rectangles, 4 * rectangles);
    m_ellipses->reserve(ellipses, 4 * ellipses);
}


void Shape::draw(View& view, const ViewportCoords& pos)
{
    // lines
    if (!m_lines->empty()) {
        init_line_shader();
        m_lines->set_uniform("u_fill_color",
                                   m_fill_color.red_f(), m_fill_color.green_f(),
                                   m_fill_color.blue_f(), m_fill_color.alpha_f());
        m_lines->set_uniform("u_outline_color",
                                   m_outline_color.red_f(), m_outline_color.green_f(),
                                   m_outline_color.blue_f(), m_outline_color.alpha_f());
        m_lines->set_uniform("u_softness", m_softness);
        m_lines->set_uniform("u_antialiasing", m_antialiasing);
        m_lines->set_shader(*m_line_shader);
        m_lines->set_blend(Primitives::BlendFunc::AlphaBlend);
        m_lines->draw(view, pos);
    }

    // rectangles
    if (!m_rectangles->empty()) {
        init_rectangle_shader();
        m_rectangles->set_uniform("u_outline_color",
                m_outline_color.red_f(), m_outline_color.green_f(),
                m_outline_color.blue_f(), m_outline_color.alpha_f());
        m_rectangles->set_uniform("u_softness", m_softness);
        m_rectangles->set_uniform("u_antialiasing", m_antialiasing);
        m_rectangles->set_shader(*m_rectangle_shader);
        m_rectangles->set_blend(Primitives::BlendFunc::AlphaBlend);
        m_rectangles->draw(view, pos);
    }

    // ellipses
    if (!m_ellipses->empty()) {
        init_ellipse_shader();
        m_ellipses->set_uniform("u_fill_color",
                m_fill_color.red_f(), m_fill_color.green_f(),
                m_fill_color.blue_f(), m_fill_color.alpha_f());
        m_ellipses->set_uniform("u_outline_color",
                m_outline_color.red_f(), m_outline_color.green_f(),
                m_outline_color.blue_f(), m_outline_color.alpha_f());
        m_ellipses->set_uniform("u_softness", m_softness);
        m_ellipses->set_uniform("u_antialiasing", m_antialiasing);
        m_ellipses->set_shader(*m_ellipse_shader);
        m_ellipses->set_blend(Primitives::BlendFunc::AlphaBlend);
        m_ellipses->draw(view, pos);
    }
}


void Shape::init_line_shader()
{
    if (m_line_shader)
        return;
    m_line_shader = m_renderer.get_or_create_shader(ShaderId::Line);
    if (m_line_shader->is_ready())
        return;

#ifdef XCI_EMBED_SHADERS
    bool res = m_line_shader->load_from_memory(
                (const char*)g_line_vert_data, g_line_vert_size,
                (const char*)g_line_frag_data, g_line_frag_size);
#else
    bool res = m_line_shader->load_from_vfs(m_renderer.vfs(),
            "shaders/line.vert", "shaders/line.frag");
#endif
    if (!res) {
        log_error("Line shader not loaded!");
    }
}


void Shape::init_rectangle_shader()
{
    if (m_rectangle_shader)
        return;
    m_rectangle_shader = m_renderer.get_or_create_shader(ShaderId::Rectangle);
    if (m_rectangle_shader->is_ready())
        return;

#ifdef XCI_EMBED_SHADERS
    bool res = m_rectangle_shader->load_from_memory(
                (const char*)g_rectangle_vert_data, g_rectangle_vert_size,
                (const char*)g_rectangle_frag_data, g_rectangle_frag_size);
#else
    bool res = m_rectangle_shader->load_from_vfs(m_renderer.vfs(),
            "shaders/rectangle.vert", "shaders/rectangle.frag");
#endif
    if (!res) {
        log_error("Rectangle shader not loaded!");
    }
}


void Shape::init_ellipse_shader()
{
    if (m_ellipse_shader)
        return;
    m_ellipse_shader = m_renderer.get_or_create_shader(ShaderId::Ellipse);
    if (m_ellipse_shader->is_ready())
        return;

#ifdef XCI_EMBED_SHADERS
    bool res = m_ellipse_shader->load_from_memory(
                (const char*)g_ellipse_vert_data, g_ellipse_vert_size,
                (const char*)g_ellipse_frag_data, g_ellipse_frag_size);
#else
    bool res = m_ellipse_shader->load_from_vfs(m_renderer.vfs(),
            "shaders/ellipse.vert", "shaders/ellipse.frag");
#endif
    if (!res) {
        log_error("Ellipse shader not loaded!");
    }
}


} // namespace xci::graphics
