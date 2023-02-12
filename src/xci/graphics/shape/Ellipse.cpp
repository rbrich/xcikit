// Ellipse.cpp created on 2018-04-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Ellipse.h"


namespace xci::graphics {


void Ellipse::add_ellipse(const FramebufferRect& rect, FramebufferPixels outline_thickness)
{
    const auto x1 = rect.x;
    const auto y1 = rect.y;
    const auto x2 = rect.x + rect.w;
    const auto y2 = rect.y + rect.h;
    const float tx = (2.0f * outline_thickness / rect.w).value;
    const float ty = (2.0f * outline_thickness / rect.h).value;
    const float ix = 1.0f + tx / (1-tx);
    const float iy = 1.0f + ty / (1-ty);
    m_primitives.begin_primitive();
    m_primitives.add_vertex({x1, y1}).uv(-ix, -iy).uv(-1.0f, -1.0f);
    m_primitives.add_vertex({x1, y2}).uv(-ix, +iy).uv(-1.0f, +1.0f);
    m_primitives.add_vertex({x2, y2}).uv(+ix, +iy).uv(+1.0f, +1.0f);
    m_primitives.add_vertex({x2, y1}).uv(+ix, -iy).uv(+1.0f, -1.0f);
    m_primitives.end_primitive();
}


void Ellipse::add_ellipse_slice(const FramebufferRect& slice, const FramebufferRect& ellipse,
                                FramebufferPixels outline_thickness)
{
    const auto x1 = slice.x;
    const auto y1 = slice.y;
    const auto x2 = slice.x + slice.w;
    const auto y2 = slice.y + slice.h;
    const float ax = (2.0f * (slice.x - ellipse.x-ellipse.w/2) / ellipse.w).value;
    const float ay = (2.0f * (slice.y - ellipse.y-ellipse.h/2) / ellipse.h).value;
    const float bx = (2.0f * (slice.x+slice.w - ellipse.x-ellipse.w/2) / ellipse.w).value;
    const float by = (2.0f * (slice.y+slice.h - ellipse.y-ellipse.h/2) / ellipse.h).value;
    const float tx = (2.0f * outline_thickness / ellipse.w).value;
    const float ty = (2.0f * outline_thickness / ellipse.h).value;
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


void ColoredEllipse::add_ellipse(const FramebufferRect& rect,
                        Color fill_color, Color outline_color, FramebufferPixels outline_thickness)
{
    const auto x1 = rect.x;
    const auto y1 = rect.y;
    const auto x2 = rect.x + rect.w;
    const auto y2 = rect.y + rect.h;
    const float tx = (2.0f * outline_thickness / rect.w).value;
    const float ty = (2.0f * outline_thickness / rect.h).value;
    const float ix = 1.0f + tx / (1-tx);
    const float iy = 1.0f + ty / (1-ty);
    m_primitives.begin_primitive();
    m_primitives.add_vertex({x1, y1}).color(fill_color).color(outline_color).uv(-ix, -iy).uv(-1.0f, -1.0f);
    m_primitives.add_vertex({x1, y2}).color(fill_color).color(outline_color).uv(-ix, +iy).uv(-1.0f, +1.0f);
    m_primitives.add_vertex({x2, y2}).color(fill_color).color(outline_color).uv(+ix, +iy).uv(+1.0f, +1.0f);
    m_primitives.add_vertex({x2, y1}).color(fill_color).color(outline_color).uv(+ix, -iy).uv(+1.0f, -1.0f);
    m_primitives.end_primitive();
}


void ColoredEllipse::add_ellipse_slice(const FramebufferRect& slice, const FramebufferRect& ellipse,
                       Color fill_color, Color outline_color, FramebufferPixels outline_thickness)
{
    const auto x1 = slice.x;
    const auto y1 = slice.y;
    const auto x2 = slice.x + slice.w;
    const auto y2 = slice.y + slice.h;
    const float ax = (2.0f * (slice.x - ellipse.x-ellipse.w/2) / ellipse.w).value;
    const float ay = (2.0f * (slice.y - ellipse.y-ellipse.h/2) / ellipse.h).value;
    const float bx = (2.0f * (slice.x+slice.w - ellipse.x-ellipse.w/2) / ellipse.w).value;
    const float by = (2.0f * (slice.y+slice.h - ellipse.y-ellipse.h/2) / ellipse.h).value;
    const float tx = (2.0f * outline_thickness / ellipse.w).value;
    const float ty = (2.0f * outline_thickness / ellipse.h).value;
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
