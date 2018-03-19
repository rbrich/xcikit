// GlSprites.cpp created on 2018-03-14, part of XCI toolkit
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

#include "GlSprites.h"
#include "GlTexture.h"
#include "GlView.h"

#include <xci/util/log.h>

// inline
#include <xci/graphics/Sprites.inl>

namespace xci {
namespace graphics {


using namespace xci::util::log;


GlSprites::GlSprites(const Texture& texture) : m_texture(texture) {}


void GlSprites::add_sprite(const Rect_f& rect, const Color& color)
{
    auto ts = m_texture.size();
    add_sprite(rect, {0, 0, ts.x, ts.y}, color);
}


// Position a sprite with cutoff from the texture
void GlSprites::add_sprite(const Rect_f& rect, const Rect_u& texrect, const Color& color)
{
    clear_gl_objects();

    float x1 = rect.x;
    float y1 = -rect.y;
    float x2 = rect.x + rect.w;
    float y2 = -rect.y - rect.h;
    float r = color.red_f();
    float g = color.green_f();
    float b = color.blue_f();
    float a = color.alpha_f();
    auto ts = m_texture.size();
    float tl = (float)texrect.left() / ts.x;
    float tr = (float)texrect.right() / ts.x;
    float tb = (float)texrect.bottom() / ts.y;
    float tt = (float)texrect.top() / ts.y;
    GLushort i = (GLushort) m_vertex_data.size();
    m_vertex_data.push_back({x2, y1, r, g, b, a, tr, tt});
    m_vertex_data.push_back({x2, y2, r, g, b, a, tr, tb});
    m_vertex_data.push_back({x1, y2, r, g, b, a, tl, tb});
    m_vertex_data.push_back({x1, y1, r, g, b, a, tl, tt});
    GLushort indices[] = {GLushort(i+0), GLushort(i+1), GLushort(i+2),
                          GLushort(i+2), GLushort(i+3), GLushort(i+0)};
    m_indices.insert(m_indices.end(), std::begin(indices), std::end(indices));
}


void GlSprites::draw(View& view, const Vec2f& pos)
{
    init_gl_objects();

    auto program = view.impl().gl_program();
    glUseProgram(program);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // projection matrix
    GLfloat xs = 2.0f / view.size().x;
    GLfloat ys = 2.0f / view.size().y;
    GLfloat xt = pos.x * xs;
    GLfloat yt = pos.y * ys;
    const GLfloat mvp[] = {
            xs,   0.0f, 0.0f, 0.0f,
            0.0f, ys,   0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            xt,  -yt,   0.0f, 1.0f,
    };

    GLint mvp_location = glGetUniformLocation(program, "u_mvp");
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);

    GLint tex_location = glGetUniformLocation(program, "u_texture");
    glUniform1i(tex_location, 0); // GL_TEXTURE0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture.impl().gl_texture());
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(m_vertex_array);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);

    glDrawElements(GL_TRIANGLES, (GLsizei) m_indices.size(), GL_UNSIGNED_SHORT, nullptr);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glUseProgram(0);
}


void GlSprites::init_gl_objects()
{
    if (m_objects_ready)
        return;

    glGenVertexArrays(1, &m_vertex_array);
    glBindVertexArray(m_vertex_array);

    glGenBuffers(1, &m_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_vertex_data.size(),
                 m_vertex_data.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) (sizeof(GLfloat) * 0));
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) (sizeof(GLfloat) * 2));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) (sizeof(GLfloat) * 6));

    glGenBuffers(1, &m_index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * m_indices.size(),
                 m_indices.data(), GL_STATIC_DRAW);

    m_objects_ready = true;
}


void GlSprites::clear_gl_objects()
{
    if (!m_objects_ready)
        return;

    glDeleteVertexArrays(1, &m_vertex_array);
    glDeleteBuffers(1, &m_vertex_buffer);
    glDeleteBuffers(1, &m_index_buffer);

    m_objects_ready = false;
}


}} // namespace xci::graphics
