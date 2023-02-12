// Line.cpp created on 2018-04-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Line.h"


namespace xci::graphics {


void Line::add_line(FramebufferCoords a, FramebufferCoords b, FramebufferPixels thickness)
{
    add_line_slice(
            { std::min(a.x, b.x), std::min(a.y, b.y),
             std::abs(a.x.value - b.x.value), std::abs(a.y.value - b.y.value) },
            a, b, thickness);
}


void Line::add_line_slice(const FramebufferRect& slice,
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
    m_primitives.begin_primitive();
    m_primitives.add_vertex({x1, y1}).uv(t1.x, t1.y);
    m_primitives.add_vertex({x1, y2}).uv(t2.x, t2.y);
    m_primitives.add_vertex({x2, y2}).uv(t3.x, t3.y);
    m_primitives.add_vertex({x2, y1}).uv(t4.x, t4.y);
    m_primitives.end_primitive();
}


// -----------------------------------------------------------------------------


void ColoredLine::add_line(FramebufferCoords a, FramebufferCoords b,
                           Color fill_color, Color outline_color, FramebufferPixels thickness)
{
    add_line_slice(
            { std::min(a.x, b.x), std::min(a.y, b.y),
             std::abs(a.x.value - b.x.value), std::abs(a.y.value - b.y.value) },
            a, b, fill_color, outline_color, thickness);
}


void ColoredLine::add_line_slice(const FramebufferRect& slice,
                                 FramebufferCoords a, FramebufferCoords b,
                                 Color fill_color, Color outline_color, FramebufferPixels thickness)
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
    m_primitives.begin_primitive();
    m_primitives.add_vertex({x1, y1}).color(fill_color).color(outline_color).uv(t1.x, t1.y);
    m_primitives.add_vertex({x1, y2}).color(fill_color).color(outline_color).uv(t2.x, t2.y);
    m_primitives.add_vertex({x2, y2}).color(fill_color).color(outline_color).uv(t3.x, t3.y);
    m_primitives.add_vertex({x2, y1}).color(fill_color).color(outline_color).uv(t4.x, t4.y);
    m_primitives.end_primitive();
}


} // namespace xci::graphics
