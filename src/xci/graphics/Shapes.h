// Shapes.h created on 2018-04-04, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_SHAPES_H
#define XCI_GRAPHICS_SHAPES_H

#include <xci/graphics/Color.h>
#include <xci/graphics/View.h>
#include <xci/util/geometry.h>

namespace xci {
namespace graphics {

using xci::util::Rect_f;
using xci::util::Vec2f;

// A collection of basic shapes: rectangles, ellipses.
// Each shape may have different size and outline width,
// but colors are uniform.

class Shapes {
public:
    explicit Shapes(const Color& fill_color,
                    const Color& outline_color = Color::White());
    ~Shapes();
    Shapes(Shapes&&) noexcept;
    Shapes& operator=(Shapes&&) noexcept;

    // Add new rectangle.
    // `rect`              - rectangle position and size
    // `outline_thickness` - the outline actually goes from edge to inside
    //                       this parameter defines how far (in display units)
    void add_rectangle(const Rect_f& rect,
                       float outline_thickness = 0);

    // Add new ellipse.
    // `rect`              - ellipse position and size
    // `outline_thickness` - the outline actually goes from edge to inside
    //                       this parameter defines how far (in display units)
    void add_ellipse(const Rect_f& rect,
                     float outline_thickness = 0);

    // Add new rounded rectangle.
    // `rect`              - position and size
    // `radius`            - corner radius
    // `outline_thickness` - the outline actually goes from edge to inside
    //                       this parameter defines how far (in display units)
    void add_rounded_rectangle(const Rect_f& rect, float radius,
                               float outline_thickness = 0);

    // Remove all shapes
    void clear();

    // Draw all shapes to `view` at `pos`.
    // Final shape position is `pos` + shapes's relative position
    void draw(View& view, const Vec2f& pos);

    class Impl;
    const Impl& impl() const { return *m_impl; }

private:
    std::unique_ptr<Impl> m_impl;
};


}} // namespace xci::graphics

#endif //XCI_GRAPHICS_SHAPES_H
