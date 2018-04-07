// GlEllipses.h created on 2018-03-24, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_GL_ELLIPSES_H
#define XCI_GRAPHICS_GL_ELLIPSES_H

#include "GlPrimitives.h"
#include <xci/graphics/Color.h>
#include <xci/graphics/View.h>
#include <xci/util/geometry.h>

#include <glad/glad.h>

#include <vector>

namespace xci {
namespace graphics {

using xci::util::Rect_f;
using xci::util::Vec2f;


class GlEllipses {
public:
    void add_ellipse(const Rect_f& rect,
                     float outline_thickness = 0);
    void add_ellipse_slice(const Rect_f& slice, const Rect_f& ellipse,
                           float outline_thickness = 0);
    void clear_ellipses();

    void draw(View& view, const Vec2f& pos,
              const Color& fill_color, const Color& outline_color,
              float antialiasing = 2, float softness = 0);

private:
    GlPrimitives m_primitives;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_GL_ELLIPSES_H
