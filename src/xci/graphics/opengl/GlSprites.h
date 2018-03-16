// GlSprites.h created on 2018-03-14, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_GL_SPRITES_H
#define XCI_GRAPHICS_GL_SPRITES_H

#include <xci/graphics/Sprites.h>
#include <xci/graphics/Texture.h>

#include <glad/glad.h>

#include <vector>

namespace xci {
namespace graphics {


class GlSprites {
public:
    explicit GlSprites(const Texture& texture);

    void add_sprite(const Vec2f& pos, const Color& color);
    void add_sprite(const Vec2f& pos, const Rect_u& texrect, const Color& color);
    void draw(View& view, const Vec2f& pos);

private:
    const Texture& m_texture;

    struct Vertex {
        GLfloat x, y;         // vertex coords
        GLfloat r, g, b, a;   // color
        GLfloat u, v;         // texture coords
    };
    std::vector<Vertex> m_vertex_data;
    std::vector<GLushort> m_indices;

    GLuint m_vertex_buffer;
};


class Sprites::Impl : public GlSprites {
public:
    explicit Impl(const Texture& texture) : GlSprites(texture) {}
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_GL_SPRITES_H
