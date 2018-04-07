// GlRectangles.cpp created on 2018-03-19, part of XCI toolkit
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

#include "GlRectangles.h"
#include "GlView.h"

#ifdef XCI_EMBED_SHADERS
#define INCBIN_PREFIX g_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "incbin.h"
INCBIN(rectangle_vert, XCI_SHARE_DIR "/shaders/rectangle.vert");
INCBIN(rectangle_frag, XCI_SHARE_DIR "/shaders/rectangle.frag");
#else
static const unsigned char* g_rectangle_vert_data = nullptr;
static const unsigned int g_rectangle_vert_size = 0;
static const unsigned char* g_rectangle_frag_data = nullptr;
static const unsigned int g_rectangle_frag_size = 0;
#endif


namespace xci {
namespace graphics {


void GlRectangles::add_rectangle(const Rect_f& rect, float outline_thickness)
{
    float x1 = rect.x;
    float y1 = -rect.y;
    float x2 = rect.x + rect.w;
    float y2 = -rect.y - rect.h;
    float tx = 2.0f * outline_thickness / rect.w;
    float ty = 2.0f * outline_thickness / rect.h;
    float ix = 1.0f + tx / (1-tx);
    float iy = 1.0f + ty / (1-ty);
    
    m_primitives.begin_primitive();
    m_primitives.add_vertex({x2, y1, +ix, -iy, +1.0f, -1.0f});
    m_primitives.add_vertex({x2, y2, +ix, +iy, +1.0f, +1.0f});
    m_primitives.add_vertex({x1, y2, -ix, +iy, -1.0f, +1.0f});
    m_primitives.add_vertex({x1, y1, -ix, -iy, -1.0f, -1.0f});
    m_primitives.end_primitive();
}


void GlRectangles::add_rectangle_slice(const Rect_f& slice, const Rect_f& rect,
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
    m_primitives.begin_primitive();
    m_primitives.add_vertex({x2, y1, cx, dy, ax, by});
    m_primitives.add_vertex({x2, y2, cx, cy, ax, ay});
    m_primitives.add_vertex({x1, y2, dx, cy, bx, ay});
    m_primitives.add_vertex({x1, y1, dx, dy, bx, by});
    m_primitives.end_primitive();
}


void GlRectangles::clear_rectangles()
{
    m_primitives.clear();
}


void GlRectangles::draw(View& view, const Vec2f& pos,
                        const Color& fill_color, const Color& outline_color,
                        float antialiasing, float softness)
{
    auto program = view.impl()
            .gl_program(GlView::ProgramId::Rectangle,
                        "shaders/rectangle.vert", "shaders/rectangle.frag",
                        (const char*)g_rectangle_vert_data, g_rectangle_vert_size,
                        (const char*)g_rectangle_frag_data, g_rectangle_frag_size);
    m_primitives.set_program(program);
    m_primitives.set_uniform("u_fill_color",
                             fill_color.red_f(), fill_color.green_f(),
                             fill_color.blue_f(), fill_color.alpha_f());
    m_primitives.set_uniform("u_outline_color",
                             outline_color.red_f(), outline_color.green_f(), 
                             outline_color.blue_f(), outline_color.alpha_f());
    m_primitives.set_uniform("u_softness", softness);
    m_primitives.set_uniform("u_antialiasing", antialiasing);
    m_primitives.draw(view, pos);
}


}} // namespace xci::graphics
