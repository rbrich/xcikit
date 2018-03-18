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
    auto program = view.impl().gl_program();

    GLuint vertex_array;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    glGenBuffers(1, &m_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_vertex_data.size(),
                 m_vertex_data.data(), GL_STATIC_DRAW);

    GLint res;
    res = glGetAttribLocation(program, "a_position");
    if (res == -1) {
        log_error("Couldn't get location of shader attribute: {}", "a_position");
        exit(1);
    }
    auto a_position = GLuint(res);
    glEnableVertexAttribArray(a_position);
    glVertexAttribPointer(a_position, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) (sizeof(GLfloat) * 0));

    res = glGetAttribLocation(program, "a_color");
    if (res == -1) {
        log_error("Couldn't get location of shader attribute: {}", "a_color");
        exit(1);
    }
    auto a_color = GLuint(res);
    glEnableVertexAttribArray(a_color);
    glVertexAttribPointer(a_color, 4, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) (sizeof(GLfloat) * 2));

    res = glGetAttribLocation(program, "a_tex_coord");
    if (res == -1) {
        log_error("Couldn't get location of shader attribute: {}", "a_tex_coord");
        exit(1);
    }
    auto a_tex_coord = GLuint(res);
    glEnableVertexAttribArray(a_tex_coord);
    glVertexAttribPointer(a_tex_coord, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) (sizeof(GLfloat) * 6));

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

    glUseProgram(program);
    GLint mvp_location = glGetUniformLocation(program, "u_mvp");
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);

    GLint tex_location = glGetUniformLocation(program, "u_texture");
    glUniform1i(tex_location, 0); // GL_TEXTURE0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture.impl().gl_texture());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawElements(GL_TRIANGLES, (GLsizei) m_indices.size(),
                   GL_UNSIGNED_SHORT, m_indices.data());

    glDisableVertexAttribArray(a_position);
    glDisableVertexAttribArray(a_color);
    glDisableVertexAttribArray(a_tex_coord);

    //glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &m_vertex_buffer);
    //glBindVertexArray(0);
    glDeleteVertexArrays(1, &vertex_array);
}


}} // namespace xci::graphics
