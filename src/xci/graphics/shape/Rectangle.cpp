// Rectangle.cpp created on 2018-04-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Rectangle.h"


namespace xci::graphics {


void Rectangle::add_rectangle(const FramebufferRect& rect, FramebufferPixels outline_thickness)
{
    const auto x1 = rect.x;
    const auto y1 = rect.y;
    const auto x2 = rect.x + rect.w;
    const auto y2 = rect.y + rect.h;
    const float tx = 2.0f * outline_thickness.value / rect.w.value;
    const float ty = 2.0f * outline_thickness.value / rect.h.value;
    const float ix = 1.0f + tx / (1.0f - tx);
    const float iy = 1.0f + ty / (1.0f - ty);
    m_primitives.begin_primitive();
    m_primitives.add_vertex({x1, y1}).uv(-ix, -iy).uv(-1.0f, -1.0f);
    m_primitives.add_vertex({x1, y2}).uv(-ix, +iy).uv(-1.0f, +1.0f);
    m_primitives.add_vertex({x2, y2}).uv(+ix, +iy).uv(+1.0f, +1.0f);
    m_primitives.add_vertex({x2, y1}).uv(+ix, -iy).uv(+1.0f, -1.0f);
    m_primitives.end_primitive();
}


void Rectangle::add_rectangle_slice(const FramebufferRect& slice, const FramebufferRect& rect,
                                    FramebufferPixels outline_thickness)
{
    const auto x1 = slice.x;
    const auto y1 = slice.y;
    const auto x2 = slice.x + slice.w;
    const auto y2 = slice.y + slice.h;
    const auto rcx = rect.x + rect.w/2;
    const auto rcy = rect.y + rect.h/2;
    const float ax = (2.0f * (slice.x - rcx) / rect.w).value;
    const float ay = (2.0f * (slice.y - rcy) / rect.h).value;
    const float bx = (2.0f * (x2 - rcx) / rect.w).value;
    const float by = (2.0f * (y2 - rcy) / rect.h).value;
    const float tx = (2.0f * outline_thickness / rect.w).value;
    const float ty = (2.0f * outline_thickness / rect.h).value;
    const float cx = ax * (1.0f + tx / (1-tx));
    const float cy = ay * (1.0f + ty / (1-ty));
    const float dx = bx * (1.0f + tx / (1-tx));
    const float dy = by * (1.0f + ty / (1-ty));
    m_primitives.begin_primitive();
    m_primitives.add_vertex({x1, y1}).uv(cx, cy).uv(ax, ay);
    m_primitives.add_vertex({x1, y2}).uv(cx, dy).uv(ax, by);
    m_primitives.add_vertex({x2, y2}).uv(dx, dy).uv(bx, by);
    m_primitives.add_vertex({x2, y1}).uv(dx, cy).uv(bx, ay);
    m_primitives.end_primitive();
}


// -----------------------------------------------------------------------------


void ColoredRectangle::add_rectangle(const FramebufferRect& rect,
                         Color fill_color, Color outline_color, FramebufferPixels outline_thickness)
{
    const auto x1 = rect.x;
    const auto y1 = rect.y;
    const auto x2 = rect.x + rect.w;
    const auto y2 = rect.y + rect.h;
    const float tx = 2.0f * outline_thickness.value / rect.w.value;
    const float ty = 2.0f * outline_thickness.value / rect.h.value;
    const float ix = 1.0f + tx / (1.0f - tx);
    const float iy = 1.0f + ty / (1.0f - ty);
    m_primitives.begin_primitive();
    m_primitives.add_vertex({x1, y1}).color(fill_color).color(outline_color).uv(-ix, -iy).uv(-1.0f, -1.0f);
    m_primitives.add_vertex({x1, y2}).color(fill_color).color(outline_color).uv(-ix, +iy).uv(-1.0f, +1.0f);
    m_primitives.add_vertex({x2, y2}).color(fill_color).color(outline_color).uv(+ix, +iy).uv(+1.0f, +1.0f);
    m_primitives.add_vertex({x2, y1}).color(fill_color).color(outline_color).uv(+ix, -iy).uv(+1.0f, -1.0f);
    m_primitives.end_primitive();
}


void ColoredRectangle::add_rectangle_slice(const FramebufferRect& slice, const FramebufferRect& rect,
                         Color fill_color, Color outline_color, FramebufferPixels outline_thickness)
{
    const auto x1 = slice.x;
    const auto y1 = slice.y;
    const auto x2 = slice.x + slice.w;
    const auto y2 = slice.y + slice.h;
    const auto rcx = rect.x + rect.w/2;
    const auto rcy = rect.y + rect.h/2;
    const float ax = (2.0f * (slice.x - rcx) / rect.w).value;
    const float ay = (2.0f * (slice.y - rcy) / rect.h).value;
    const float bx = (2.0f * (x2 - rcx) / rect.w).value;
    const float by = (2.0f * (y2 - rcy) / rect.h).value;
    const float tx = (2.0f * outline_thickness / rect.w).value;
    const float ty = (2.0f * outline_thickness / rect.h).value;
    const float cx = ax * (1.0f + tx / (1-tx));
    const float cy = ay * (1.0f + ty / (1-ty));
    const float dx = bx * (1.0f + tx / (1-tx));
    const float dy = by * (1.0f + ty / (1-ty));
    m_primitives.begin_primitive();
    m_primitives.add_vertex({x1, y1}).color(fill_color).color(outline_color).uv(cx, cy).uv(ax, ay);
    m_primitives.add_vertex({x1, y2}).color(fill_color).color(outline_color).uv(cx, dy).uv(ax, by);
    m_primitives.add_vertex({x2, y2}).color(fill_color).color(outline_color).uv(dx, dy).uv(bx, by);
    m_primitives.add_vertex({x2, y1}).color(fill_color).color(outline_color).uv(dx, cy).uv(bx, ay);
    m_primitives.end_primitive();
}


} // namespace xci::graphics
