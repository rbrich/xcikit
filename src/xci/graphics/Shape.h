// Shape.h created on 2018-04-04, part of XCI toolkit
// Copyright 2018, 2019 Radek Brich
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

#ifndef XCI_GRAPHICS_SHAPES_H
#define XCI_GRAPHICS_SHAPES_H

#include "Primitives.h"
#include "Renderer.h"
#include <xci/graphics/Color.h>
#include <xci/graphics/View.h>
#include <xci/core/geometry.h>

namespace xci::graphics {

using xci::core::Rect_f;
using xci::core::Vec2f;


/// A collection of one of basic shapes: rectangles, ellipses, lines.
/// Each shape may have different size and outline width,
/// but colors are uniform.
class Shape {
public:
    explicit Shape(Renderer& renderer,
                   const Color& fill_color = Color::Black(),
                   const Color& outline_color = Color::White());

    void set_fill_color(const Color& fill_color) { m_fill_color = fill_color; }
    void set_outline_color(const Color& outline_color) { m_outline_color = outline_color; }
    void set_antialiasing(float antialiasing) { m_antialiasing = antialiasing; }
    void set_softness(float softness) { m_softness = softness; }

    const Color& fill_color() const { return m_fill_color; }
    const Color& outline_color() const { return m_outline_color; }

    // Add a slice of infinite line
    // `a`, `b`            - two points to define the line
    // `slice`             - rectangular region in which the line is visible
    // `thickness`         - line width, measured perpendicularly from a-b
    //
    //   ---- a --- b ----
    //                    > thickness
    //   -----------------
    void add_line_slice(const ViewportRect& slice,
                        const ViewportCoords& a, const ViewportCoords& b,
                        ViewportUnits thickness);

    // Add new rectangle.
    // `rect`              - rectangle position and size
    // `outline_thickness` - the outline actually goes from edge to inside
    //                       this parameter defines how far (in viewport units)
    void add_rectangle(const ViewportRect& rect,
                       ViewportUnits outline_thickness = 0);
    void add_rectangle_slice(const ViewportRect& slice, const ViewportRect& rect,
                             ViewportUnits outline_thickness = 0);

    // Add new ellipse.
    // `rect`              - ellipse position and size
    // `outline_thickness` - the outline actually goes from edge to inside
    //                       this parameter defines how far (in display units)
    void add_ellipse(const ViewportRect& rect,
                     ViewportUnits outline_thickness = 0);
    void add_ellipse_slice(const ViewportRect& slice, const ViewportRect& ellipse,
                           ViewportUnits outline_thickness = 0);

    // Add new rounded rectangle.
    // `rect`              - position and size
    // `radius`            - corner radius
    // `outline_thickness` - the outline actually goes from edge to inside
    //                       this parameter defines how far (in display units)
    void add_rounded_rectangle(const ViewportRect& rect, ViewportUnits radius,
                               ViewportUnits outline_thickness = 0);

    // Reserve memory for a number of `lines`, `rectangles`, `ellipses`.
    void reserve(size_t lines, size_t rectangles, size_t ellipses);

    // Remove all shapes and clear all state (colors etc.)
    void clear();

    // Update shapes attributes according to settings (color etc.)
    void update();

    // Draw all shapes to `view` at `pos`.
    // Final shape position is `pos` + shapes's relative position
    void draw(View& view, const ViewportCoords& pos);

private:
    Renderer& m_renderer;

    Color m_fill_color;
    Color m_outline_color;
    float m_antialiasing = 0;
    float m_softness = 0;

    Primitives m_lines;
    Primitives m_rectangles;
    Primitives m_ellipses;

    Shader& m_line_shader;
    Shader& m_rectangle_shader;
    Shader& m_ellipse_shader;
};


} // namespace xci::graphics

#endif //XCI_GRAPHICS_SHAPES_H
