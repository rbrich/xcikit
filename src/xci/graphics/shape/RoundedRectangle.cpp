// RoundedRectangle.cpp created on 2018-04-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "RoundedRectangle.h"


namespace xci::graphics {


void RoundedRectangle::add_rounded_rectangle(const FramebufferRect& rect, FramebufferPixels radius,
                                             FramebufferPixels outline_thickness)
{
    const auto x1 = rect.x;
    const auto y1 = rect.y;
    const auto x2 = rect.x + rect.w;
    const auto y2 = rect.y + rect.h;
    const float sx = 0.5f * rect.w.value;
    const float sy = 0.5f * rect.h.value;
    const float t = outline_thickness.value;
    const float r = radius.value;
    m_primitives.begin_primitive();
    m_primitives.add_vertex({x1, y1}).uv(-1.0f, -1.0f).uv(sx, sy).uv(t, r);
    m_primitives.add_vertex({x1, y2}).uv(-1.0f, +1.0f).uv(sx, sy).uv(t, r);
    m_primitives.add_vertex({x2, y2}).uv(+1.0f, +1.0f).uv(sx, sy).uv(t, r);
    m_primitives.add_vertex({x2, y1}).uv(+1.0f, -1.0f).uv(sx, sy).uv(t, r);
    m_primitives.end_primitive();
}


void RoundedRectangle::add_rounded_rectangle_slice(
        const FramebufferRect& slice, const FramebufferRect& rect, FramebufferPixels radius,
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
    const float sx = 0.5f * rect.w.value;
    const float sy = 0.5f * rect.h.value;
    const float t = outline_thickness.value;
    const float r = radius.value;
    m_primitives.begin_primitive();
    m_primitives.add_vertex({x1, y1}).uv(ax, ay).uv(sx, sy).uv(t, r);
    m_primitives.add_vertex({x1, y2}).uv(ax, by).uv(sx, sy).uv(t, r);
    m_primitives.add_vertex({x2, y2}).uv(bx, by).uv(sx, sy).uv(t, r);
    m_primitives.add_vertex({x2, y1}).uv(bx, ay).uv(sx, sy).uv(t, r);
    m_primitives.end_primitive();
}



// -----------------------------------------------------------------------------


void ColoredRoundedRectangle::add_rounded_rectangle(const FramebufferRect& rect, FramebufferPixels radius,
                         Color fill_color, Color outline_color, FramebufferPixels outline_thickness)
{
    const auto x1 = rect.x;
    const auto y1 = rect.y;
    const auto x2 = rect.x + rect.w;
    const auto y2 = rect.y + rect.h;
    const float sx = 0.5f * rect.w.value;
    const float sy = 0.5f * rect.h.value;
    const float t = outline_thickness.value;
    const float r = radius.value;
    m_primitives.begin_primitive();
    m_primitives.add_vertex({x1, y1}).color(fill_color).color(outline_color).uv(-1.0f, -1.0f).uv(sx, sy).uv(t, r);
    m_primitives.add_vertex({x1, y2}).color(fill_color).color(outline_color).uv(-1.0f, +1.0f).uv(sx, sy).uv(t, r);
    m_primitives.add_vertex({x2, y2}).color(fill_color).color(outline_color).uv(+1.0f, +1.0f).uv(sx, sy).uv(t, r);
    m_primitives.add_vertex({x2, y1}).color(fill_color).color(outline_color).uv(+1.0f, -1.0f).uv(sx, sy).uv(t, r);
    m_primitives.end_primitive();
}


void ColoredRoundedRectangle::add_rounded_rectangle_slice(
        const FramebufferRect& slice, const FramebufferRect& rect, FramebufferPixels radius,
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
    const float sx = 0.5f * rect.w.value;
    const float sy = 0.5f * rect.h.value;
    const float t = outline_thickness.value;
    const float r = radius.value;
    m_primitives.begin_primitive();
    m_primitives.add_vertex({x1, y1}).color(fill_color).color(outline_color).uv(ax, ay).uv(sx, sy).uv(t, r);
    m_primitives.add_vertex({x1, y2}).color(fill_color).color(outline_color).uv(ax, by).uv(sx, sy).uv(t, r);
    m_primitives.add_vertex({x2, y2}).color(fill_color).color(outline_color).uv(bx, by).uv(sx, sy).uv(t, r);
    m_primitives.add_vertex({x2, y1}).color(fill_color).color(outline_color).uv(bx, ay).uv(sx, sy).uv(t, r);
    m_primitives.end_primitive();
}


} // namespace xci::graphics
