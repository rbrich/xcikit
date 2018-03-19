// GlRectangles.cpp created on 2018-03-19, part of XCI toolkit
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

#include "GlRectangles.h"
#include "GlView.h"

// inline
#include <xci/graphics/Rectangles.inl>

namespace xci {
namespace graphics {


GlRectangles::GlRectangles(const Color& fill_color, const Color& outline_color)
    : m_fill_color(fill_color), m_outline_color(outline_color)
{}


void GlRectangles::add_rectangle(const Rect_f& rect, float outline_width)
{
    clear_gl_objects();

    float x1 = rect.x;
    float y1 = -rect.y;
    float x2 = rect.x + rect.w;
    float y2 = -rect.y - rect.h;
    float tx = 1.0f + outline_width / rect.w;
    float ty = 1.0f + outline_width / rect.h;
    auto i = (GLushort) m_vertex_data.size();
    m_vertex_data.push_back({x2, y1, +tx, -ty});
    m_vertex_data.push_back({x2, y2, +tx, +ty});
    m_vertex_data.push_back({x1, y2, -tx, +ty});
    m_vertex_data.push_back({x1, y1, -tx, -ty});
    GLushort indices[] = {GLushort(i+0), GLushort(i+1), GLushort(i+2),
                          GLushort(i+2), GLushort(i+3), GLushort(i+0)};
    m_indices.insert(m_indices.end(), std::begin(indices), std::end(indices));
}


void GlRectangles::draw(View& view, const Vec2f& pos)
{
    init_gl_objects();

    auto program = view.impl().gl_program_rectangle();
    glUseProgram(program);
    glEnableVertexAttribArray(0);
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

    GLint u_mvp = glGetUniformLocation(program, "u_mvp");
    glUniformMatrix4fv(u_mvp, 1, GL_FALSE, (const GLfloat*) mvp);

    GLint u_fill_color = glGetUniformLocation(program, "u_fill_color");
    glUniform4f(u_fill_color, m_fill_color.red_f(), m_fill_color.green_f(),
                              m_fill_color.blue_f(), m_fill_color.alpha_f());

    GLint u_outline_color = glGetUniformLocation(program, "u_outline_color");
    glUniform4f(u_outline_color, m_outline_color.red_f(), m_outline_color.green_f(),
                m_outline_color.blue_f(), m_outline_color.alpha_f());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(m_vertex_array);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);

    glDrawElements(GL_TRIANGLES, (GLsizei) m_indices.size(), GL_UNSIGNED_SHORT, nullptr);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(2);
    glUseProgram(0);
}


void GlRectangles::init_gl_objects()
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
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) (sizeof(GLfloat) * 2));

    glGenBuffers(1, &m_index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * m_indices.size(),
                 m_indices.data(), GL_STATIC_DRAW);

    m_objects_ready = true;
}


void GlRectangles::clear_gl_objects()
{
    if (!m_objects_ready)
        return;

    glDeleteVertexArrays(1, &m_vertex_array);
    glDeleteBuffers(1, &m_vertex_buffer);
    glDeleteBuffers(1, &m_index_buffer);

    m_objects_ready = false;
}


}} // namespace xci::graphics
