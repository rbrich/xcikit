// GlShapes.cpp created on 2018-04-04, part of XCI toolkit
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

#include "GlShapes.h"

// inline
#include <xci/graphics/Shapes.inl>

namespace xci {
namespace graphics {


GlShapes::GlShapes(const Color& fill_color, const Color& outline_color)
        : m_fill_color(fill_color), m_outline_color(outline_color)
{}


void GlShapes::add_rectangle(const Rect_f& rect, float outline_thickness)
{
    m_rectangles.add_rectangle(rect, outline_thickness);
}


void GlShapes::add_ellipse(const Rect_f& rect, float outline_thickness)
{
    m_ellipses.add_ellipse(rect, outline_thickness);
}

void
GlShapes::add_rounded_rectangle(const Rect_f& rect, float radius,
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
    m_ellipses.add_ellipse_slice({x, y, r, r},
                                 {x, y, rr, rr},
                                 outline_thickness);
    m_ellipses.add_ellipse_slice({x+w-r, y, r, r},
                                 {x+w-rr, y, rr, rr},
                                 outline_thickness);
    m_ellipses.add_ellipse_slice({x, y+h-r, r, r},
                                 {x, y+h-rr, rr, rr},
                                 outline_thickness);
    m_ellipses.add_ellipse_slice({x+w-r, y+h-r, r, r},
                                 {x+w-rr, y+h-rr, rr, rr},
                                 outline_thickness);
    m_rectangles.add_rectangle_slice({x+r, y, w-rr, r},
                                     rect, outline_thickness);
    m_rectangles.add_rectangle_slice({x+r, y+h-r, w-rr, r},
                                     rect, outline_thickness);
    m_rectangles.add_rectangle_slice({x, y+r, w, h-rr},
                                     rect, outline_thickness);
/*
    m_rectangles.add_rectangle_slice({x+r, y, r, h},
                                     rect, outline_thickness);
    m_rectangles.add_rectangle_slice({x+rr, y, r, h/2},
                                     {x,y,w,h/2}, outline_thickness);
    m_rectangles.add_rectangle_slice({x+3*r, y, r, h/4},
                                     {x,y,w,h/4}, outline_thickness);*/
}

void GlShapes::clear()
{
    m_rectangles.clear_rectangles();
    m_ellipses.clear_ellipses();
}


void GlShapes::draw(View& view, const Vec2f& pos)
{
    float softness = 10;
    m_rectangles.draw(view, pos, m_fill_color, m_outline_color, softness);
    m_ellipses.draw(view, pos, m_fill_color, m_outline_color, softness);
}


}} // namespace xci::graphics
