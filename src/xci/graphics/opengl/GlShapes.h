// GlShapes.h created on 2018-04-04, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_GL_SHAPES_H
#define XCI_GRAPHICS_GL_SHAPES_H

#include <xci/graphics/Shapes.h>
#include <xci/graphics/opengl/GlRectangles.h>
#include <xci/graphics/opengl/GlEllipses.h>

#include <glad/glad.h>

#include <vector>

namespace xci {
namespace graphics {


class GlShapes {
public:
    explicit GlShapes(const Color& fill_color,
                      const Color& outline_color = Color::White(),
                      float antialiasing = 0,
                      float softness = 0);

    void add_rectangle(const Rect_f& rect,
                       float outline_thickness = 0);

    void add_ellipse(const Rect_f& rect,
                     float outline_thickness = 0);

    void add_rounded_rectangle(const Rect_f& rect, float radius,
                               float outline_thickness = 0);

    void clear();

    void draw(View& view, const Vec2f& pos);

private:
    Color m_fill_color;
    Color m_outline_color;
    float m_softness;
    float m_antialiasing;

    GlRectangles m_rectangles;
    GlEllipses m_ellipses;
};


class Shapes::Impl : public GlShapes {
public:
    explicit Impl(const Color& fill_color, const Color& outline_color,
                  float antialiasing, float softness)
            : GlShapes(fill_color, outline_color, antialiasing, softness) {}
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_GL_SHAPES_H
