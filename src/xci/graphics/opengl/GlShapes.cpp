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


void GlShapes::clear()
{
    m_rectangles.clear_rectangles();
    m_ellipses.clear_ellipses();
}


void GlShapes::draw(View& view, const Vec2f& pos)
{
    m_rectangles.draw(view, pos, m_fill_color, m_outline_color);
    m_ellipses.draw(view, pos, m_fill_color, m_outline_color);
}


}} // namespace xci::graphics
