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
    // The shape is composed of 7-slice pattern:
    // corner ellipse slices and center rectangle slices
    auto x = rect.x;
    auto y = rect.y;
    auto w = rect.w;
    auto h = rect.h;
    auto r = std::max(radius, outline_thickness * 1.1f);
    auto rr = 2 * r;
    m_ellipses.add_ellipse_slice({x,     y,     r, r}, {x,      y,      rr, rr}, outline_thickness);
    m_ellipses.add_ellipse_slice({x+w-r, y,     r, r}, {x+w-rr, y,      rr, rr}, outline_thickness);
    m_ellipses.add_ellipse_slice({x,     y+h-r, r, r}, {x,      y+h-rr, rr, rr}, outline_thickness);
    m_ellipses.add_ellipse_slice({x+w-r, y+h-r, r, r}, {x+w-rr, y+h-rr, rr, rr}, outline_thickness);
    m_rectangles.add_rectangle_slice({x+r, y,     w-rr, r}, rect, outline_thickness);
    m_rectangles.add_rectangle_slice({x+r, y+h-r, w-rr, r}, rect, outline_thickness);
    m_rectangles.add_rectangle_slice({x,   y+r,   w, h-rr}, rect, outline_thickness);
}



// -----------------------------------------------------------------------------


void ColoredRoundedRectangle::add_rounded_rectangle(const FramebufferRect& rect, FramebufferPixels radius,
                         Color fill_color, Color outline_color, FramebufferPixels outline_thickness)
{
    // The shape is composed of 7-slice pattern:
    // corner ellipse slices and center rectangle slices
    auto x = rect.x;
    auto y = rect.y;
    auto w = rect.w;
    auto h = rect.h;
    auto r = std::max(radius, outline_thickness * 1.1f);
    auto rr = 2 * r;
    m_ellipses.add_ellipse_slice({x,     y,     r, r}, {x,      y,      rr, rr}, fill_color, outline_color, outline_thickness);
    m_ellipses.add_ellipse_slice({x+w-r, y,     r, r}, {x+w-rr, y,      rr, rr}, fill_color, outline_color, outline_thickness);
    m_ellipses.add_ellipse_slice({x,     y+h-r, r, r}, {x,      y+h-rr, rr, rr}, fill_color, outline_color, outline_thickness);
    m_ellipses.add_ellipse_slice({x+w-r, y+h-r, r, r}, {x+w-rr, y+h-rr, rr, rr}, fill_color, outline_color, outline_thickness);
    m_rectangles.add_rectangle_slice({x+r, y,     w-rr, r}, rect, fill_color, outline_color, outline_thickness);
    m_rectangles.add_rectangle_slice({x+r, y+h-r, w-rr, r}, rect, fill_color, outline_color, outline_thickness);
    m_rectangles.add_rectangle_slice({x,   y+r,   w, h-rr}, rect, fill_color, outline_color, outline_thickness);
}


} // namespace xci::graphics
