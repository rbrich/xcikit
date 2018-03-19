// GlView.h created on 2018-03-14, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_GL_VIEW_H
#define XCI_GRAPHICS_GL_VIEW_H

#include <xci/graphics/View.h>

#include <glad/glad.h>

namespace xci {
namespace graphics {


class GlView {
public:
    explicit GlView(Vec2u pixel_size);
    ~GlView();

    void resize(Vec2u pixel_size);
    Vec2f size() const { return m_size; }
    Vec2u pixel_size() const { return m_pixel_size; }

    // access native handles
    GLuint gl_program_rectangle();
    GLuint gl_program_sprite();

private:
    Vec2f m_size;       // eg. {2.666, 2.0}
    Vec2u m_pixel_size; // eg. {800, 600}
    GLuint m_rectangle_program = 0;
    GLuint m_sprite_program = 0;
};


class View::Impl : public GlView {
public:
    explicit Impl(Vec2u size) : GlView(size) {}
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_GL_VIEW_H
