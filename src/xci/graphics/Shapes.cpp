// Shapes.cpp created on 2018-04-04, part of XCI toolkit
// Copyright 2018 Radek Brich
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

#include "Shapes.h"
#include "Renderer.h"
#include <xci/util/log.h>

#ifdef XCI_EMBED_SHADERS
#define INCBIN_PREFIX g_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "incbin.h"
INCBIN(rectangle_vert, XCI_SHARE_DIR "/shaders/rectangle.vert");
INCBIN(rectangle_frag, XCI_SHARE_DIR "/shaders/rectangle.frag");
INCBIN(ellipse_vert, XCI_SHARE_DIR "/shaders/ellipse.vert");
INCBIN(ellipse_frag, XCI_SHARE_DIR "/shaders/ellipse.frag");
#endif

namespace xci {
namespace graphics {

using namespace xci::util::log;


Shapes::Shapes(const Color& fill_color, const Color& outline_color,
               float antialiasing, float softness)
        : m_fill_color(fill_color), m_outline_color(outline_color),
          m_antialiasing(antialiasing), m_softness(softness),
          m_rectangles(Primitives::VertexFormat::V2T22),
          m_ellipses(Primitives::VertexFormat::V2T22)
{}


Shapes::~Shapes() = default;
Shapes::Shapes(Shapes&&) noexcept = default;
Shapes& Shapes::operator=(Shapes&&) noexcept = default;


void Shapes::add_rectangle(const Rect_f& rect, float outline_thickness)
{
    float x1 = rect.x;
    float y1 = -rect.y;
    float x2 = rect.x + rect.w;
    float y2 = -rect.y - rect.h;
    float tx = 2.0f * outline_thickness / rect.w;
    float ty = 2.0f * outline_thickness / rect.h;
    float ix = 1.0f + tx / (1-tx);
    float iy = 1.0f + ty / (1-ty);

    m_rectangles.begin_primitive();
    m_rectangles.add_vertex(x2, y1, +ix, -iy, +1.0f, -1.0f);
    m_rectangles.add_vertex(x2, y2, +ix, +iy, +1.0f, +1.0f);
    m_rectangles.add_vertex(x1, y2, -ix, +iy, -1.0f, +1.0f);
    m_rectangles.add_vertex(x1, y1, -ix, -iy, -1.0f, -1.0f);
    m_rectangles.end_primitive();
}


void Shapes::add_rectangle_slice(const Rect_f& slice, const Rect_f& rect,
                                   float outline_thickness)
{
    float x1 = slice.x;
    float y1 = -slice.y;
    float x2 = slice.x + slice.w;
    float y2 = -slice.y - slice.h;
    float ax = 2 * (slice.x+slice.w - rect.x-rect.w/2) / rect.w;
    float ay = 2 * (slice.y+slice.h - rect.y-rect.h/2) / rect.h;
    float bx = 2 * (slice.x - rect.x-rect.w/2) / rect.w;
    float by = 2 * (slice.y - rect.y-rect.h/2) / rect.h;
    float tx = 2.0f * outline_thickness / rect.w;
    float ty = 2.0f * outline_thickness / rect.h;
    float cx = ax * (1.0f + tx / (1-tx));
    float cy = ay * (1.0f + ty / (1-ty));
    float dx = bx * (1.0f + tx / (1-tx));
    float dy = by * (1.0f + ty / (1-ty));
    m_rectangles.begin_primitive();
    m_rectangles.add_vertex(x2, y1, cx, dy, ax, by);
    m_rectangles.add_vertex(x2, y2, cx, cy, ax, ay);
    m_rectangles.add_vertex(x1, y2, dx, cy, bx, ay);
    m_rectangles.add_vertex(x1, y1, dx, dy, bx, by);
    m_rectangles.end_primitive();
}


void Shapes::add_ellipse(const Rect_f& rect, float outline_thickness)
{
    float x1 = rect.x;
    float y1 = -rect.y;
    float x2 = rect.x + rect.w;
    float y2 = -rect.y - rect.h;
    float tx = 2.0f * outline_thickness / rect.w;
    float ty = 2.0f * outline_thickness / rect.h;
    float ix = 1.0f + tx / (1-tx);
    float iy = 1.0f + ty / (1-ty);
    m_ellipses.begin_primitive();
    m_ellipses.add_vertex(x2, y1, +ix, -iy, +1.0f, -1.0f);
    m_ellipses.add_vertex(x2, y2, +ix, +iy, +1.0f, +1.0f);
    m_ellipses.add_vertex(x1, y2, -ix, +iy, -1.0f, +1.0f);
    m_ellipses.add_vertex(x1, y1, -ix, -iy, -1.0f, -1.0f);
    m_ellipses.end_primitive();
}


void Shapes::add_ellipse_slice(const Rect_f& slice, const Rect_f& ellipse,
                               float outline_thickness)
{
    float x1 = slice.x;
    float y1 = -slice.y;
    float x2 = slice.x + slice.w;
    float y2 = -slice.y - slice.h;
    float ax = 2 * (slice.x+slice.w - ellipse.x-ellipse.w/2) / ellipse.w;
    float ay = 2 * (slice.y+slice.h - ellipse.y-ellipse.h/2) / ellipse.h;
    float bx = 2 * (slice.x - ellipse.x-ellipse.w/2) / ellipse.w;
    float by = 2 * (slice.y - ellipse.y-ellipse.h/2) / ellipse.h;
    float tx = 2.0f * outline_thickness / ellipse.w;
    float ty = 2.0f * outline_thickness / ellipse.h;
    float cx = ax * (1.0f + tx / (1-tx));
    float cy = ay * (1.0f + ty / (1-ty));
    float dx = bx * (1.0f + tx / (1-tx));
    float dy = by * (1.0f + ty / (1-ty));
    m_ellipses.begin_primitive();
    m_ellipses.add_vertex(x2, y1, cx, dy, ax, by);
    m_ellipses.add_vertex(x2, y2, cx, cy, ax, ay);
    m_ellipses.add_vertex(x1, y2, dx, cy, bx, ay);
    m_ellipses.add_vertex(x1, y1, dx, dy, bx, by);
    m_ellipses.end_primitive();
}


void
Shapes::add_rounded_rectangle(const Rect_f& rect, float radius,
                              float outline_thickness)
{
    // the shape is composed from 7-slice pattern:
    // corner ellipse slices and center rectangle slices
    float x = rect.x;
    float y = rect.y;
    float w = rect.w;
    float h = rect.h;
    float r = radius;
    float rr = 2 * r;
    add_ellipse_slice({x,     y,     r, r}, {x,      y,      rr, rr}, outline_thickness);
    add_ellipse_slice({x+w-r, y,     r, r}, {x+w-rr, y,      rr, rr}, outline_thickness);
    add_ellipse_slice({x,     y+h-r, r, r}, {x,      y+h-rr, rr, rr}, outline_thickness);
    add_ellipse_slice({x+w-r, y+h-r, r, r}, {x+w-rr, y+h-rr, rr, rr}, outline_thickness);
    add_rectangle_slice({x+r, y,     w-rr, r}, rect, outline_thickness);
    add_rectangle_slice({x+r, y+h-r, w-rr, r}, rect, outline_thickness);
    add_rectangle_slice({x,   y+r,   w, h-rr}, rect, outline_thickness);
}

void Shapes::clear()
{
    m_rectangles.clear();
    m_ellipses.clear();
}


void Shapes::draw(View& view, const Vec2f& pos)
{
    // rectangles
    if (!m_rectangles.empty()) {
        init_rectangle_shader();
        m_rectangles.set_shader(m_rectangle_shader);
        m_rectangles.set_uniform("u_fill_color",
                                 m_fill_color.red_f(), m_fill_color.green_f(),
                                 m_fill_color.blue_f(), m_fill_color.alpha_f());
        m_rectangles.set_uniform("u_outline_color",
                                 m_outline_color.red_f(), m_outline_color.green_f(),
                                 m_outline_color.blue_f(), m_outline_color.alpha_f());
        m_rectangles.set_uniform("u_softness", m_softness);
        m_rectangles.set_uniform("u_antialiasing", m_antialiasing);
        m_rectangles.draw(view, pos);
    }

    // ellipses
    if (!m_ellipses.empty()) {
        init_ellipse_shader();
        m_ellipses.set_shader(m_ellipse_shader);
        m_ellipses.set_uniform("u_fill_color",
                               m_fill_color.red_f(), m_fill_color.green_f(),
                               m_fill_color.blue_f(), m_fill_color.alpha_f());
        m_ellipses.set_uniform("u_outline_color",
                               m_outline_color.red_f(), m_outline_color.green_f(),
                               m_outline_color.blue_f(), m_outline_color.alpha_f());
        m_ellipses.set_uniform("u_softness", m_softness);
        m_ellipses.set_uniform("u_antialiasing", m_antialiasing);
        m_ellipses.draw(view, pos);
    }
}


void Shapes::init_rectangle_shader()
{
    if (m_rectangle_shader)
        return;
    auto& renderer = Renderer::default_renderer();
    m_rectangle_shader = renderer.new_shader(Renderer::ShaderId::Rectangle);

#ifdef XCI_EMBED_SHADERS
    bool res = renderer.shader_load_from_memory(m_rectangle_shader,
                (const char*)g_rectangle_vert_data, g_rectangle_vert_size,
                (const char*)g_rectangle_frag_data, g_rectangle_frag_size);
)
#else
    bool res = renderer.shader_load_from_file(m_rectangle_shader,
                "shaders/rectangle.vert", "shaders/rectangle.frag");
#endif
    if (!res) {
        log_error("Rectangle shader not loaded!");
    }
}


void Shapes::init_ellipse_shader()
{
    if (m_ellipse_shader)
        return;
    auto& renderer = Renderer::default_renderer();
    m_ellipse_shader = renderer.new_shader(Renderer::ShaderId::Ellipse);

#ifdef XCI_EMBED_SHADERS
    bool res = renderer.shader_load_from_memory(m_ellipse_shader,
                (const char*)g_ellipse_vert_data, g_ellipse_vert_size,
                (const char*)g_ellipse_frag_data, g_ellipse_frag_size);
)
#else
    bool res = renderer.shader_load_from_file(m_ellipse_shader,
                "shaders/ellipse.vert", "shaders/ellipse.frag");
#endif
    if (!res) {
        log_error("Ellipse shader not loaded!");
    }
}


}} // namespace xci::graphics
