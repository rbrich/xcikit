// Rectangles.h created on 2018-03-19, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_RECTANGLES_H
#define XCI_GRAPHICS_RECTANGLES_H

#include <xci/graphics/Color.h>
#include <xci/graphics/View.h>
#include <xci/util/geometry.h>

namespace xci {
namespace graphics {

using xci::util::Rect_f;
using xci::util::Vec2f;

// A collection of plain rectangles. Each rectangle may have
// different size and outline width, but colors are uniform.
// The rectangles are alpha blended.

class Rectangles {
public:
    explicit Rectangles(const Color& fill_color,
                        const Color& outline_color = Color::White());
    ~Rectangles();
    Rectangles(Rectangles&&) noexcept;
    Rectangles& operator=(Rectangles&&) noexcept;

    // Add new rectangle.
    // `rect`          - rectangle position and size
    // `outline_width` - width of the outline (display units)
    void add_rectangle(const Rect_f& rect,
                       float outline_width = 0);

    // Draw all rectangles to `view` at `pos`.
    // Final rectangle position is `pos` + rectangle's relative position
    void draw(View& view, const Vec2f& pos);

    class Impl;
    const Impl& impl() const { return *m_impl; }

private:
    std::unique_ptr<Impl> m_impl;
};


}} // namespace xci::graphics


#endif // XCI_GRAPHICS_RECTANGLES_H
