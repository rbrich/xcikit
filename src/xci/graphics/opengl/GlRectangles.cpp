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


static const char* c_vertex_shader = R"~~~(
#version 330

uniform mat4 u_mvp;

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_border_inner;

out vec2 v_border_inner;

void main() {
    gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);
    v_border_inner = a_border_inner;
}
)~~~";


static const char* c_fragment_shader = R"~~~(
#version 330

uniform vec4 u_fill_color;
uniform vec4 u_outline_color;

in vec2 v_border_inner;

out vec4 o_color;

void main() {
    // >1 = outline, <1 = fill
    float r = max(abs(v_border_inner.x), abs(v_border_inner.y));
    float alpha = step(1, r);
    o_color = mix(u_fill_color, u_outline_color, alpha);
}
)~~~";


GlRectangles::GlRectangles(const Color& fill_color, const Color& outline_color)
    : m_fill_color(fill_color), m_outline_color(outline_color)
{}


void GlRectangles::add_rectangle(const Rect_f& rect, float outline_thickness)
{
    clear_gl_objects();

    float x1 = rect.x;
    float y1 = -rect.y;
    float x2 = rect.x + rect.w;
    float y2 = -rect.y - rect.h;
    float tx = 1.0f + 2.0f * outline_thickness / rect.w;
    float ty = 1.0f + 2.0f * outline_thickness / rect.h;
    auto i = (GLushort) m_vertex_data.size();
    m_vertex_data.push_back({x2, y1, +tx, -ty});
    m_vertex_data.push_back({x2, y2, +tx, +ty});
    m_vertex_data.push_back({x1, y2, -tx, +ty});
    m_vertex_data.push_back({x1, y1, -tx, -ty});
    m_elem_first.push_back(i);
    m_elem_size.push_back(4);
}


void GlRectangles::clear_rectangles()
{
    m_vertex_data.clear();
    m_elem_first.clear();
    m_elem_size.clear();
}


void GlRectangles::draw(View& view, const Vec2f& pos)
{
    init_gl_objects();

    auto program = view.impl().gl_program_from_string(
            GlView::ProgramId::Rectangle, c_vertex_shader, c_fragment_shader);
    glUseProgram(program);
    glBindVertexArray(m_vertex_array);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

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

    glMultiDrawArrays(GL_TRIANGLE_FAN, m_elem_first.data(), m_elem_size.data(),
                      (GLsizei) m_elem_size.size());

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
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
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) (sizeof(GLfloat) * 2));

    m_objects_ready = true;
}


void GlRectangles::clear_gl_objects()
{
    if (!m_objects_ready)
        return;

    glDeleteVertexArrays(1, &m_vertex_array);
    glDeleteBuffers(1, &m_vertex_buffer);

    m_objects_ready = false;
}


}} // namespace xci::graphics
