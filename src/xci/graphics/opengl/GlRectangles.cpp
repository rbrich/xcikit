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


void GlRectangles::add_rectangle(const Rect_f& rect, float outline_thickness)
{
    clear_gl_objects();

    float x1 = rect.x;
    float y1 = -rect.y;
    float x2 = rect.x + rect.w;
    float y2 = -rect.y - rect.h;
    float tx = 2.0f * outline_thickness / rect.w;
    float ty = 2.0f * outline_thickness / rect.h;
    float ix = 1.0f + tx / (1-tx);
    float iy = 1.0f + ty / (1-ty);
    auto i = (GLushort) m_vertex_data.size();
    m_vertex_data.push_back({x2, y1, +ix, -iy, +1.0f, -1.0f});
    m_vertex_data.push_back({x2, y2, +ix, +iy, +1.0f, +1.0f});
    m_vertex_data.push_back({x1, y2, -ix, +iy, -1.0f, +1.0f});
    m_vertex_data.push_back({x1, y1, -ix, -iy, -1.0f, -1.0f});
    m_elem_first.push_back(i);
    m_elem_size.push_back(4);
}


void GlRectangles::add_rectangle_slice(const Rect_f& slice, const Rect_f& rect,
                                       float outline_thickness)
{
    clear_gl_objects();

    float x1 = slice.x;
    float y1 = -slice.y;
    float x2 = slice.x + slice.w;
    float y2 = -slice.y - slice.h;
    float ax = 2 * (slice.x+slice.w - rect.x-rect.w/2) / rect.w;
    float ay = 2 * (slice.y+slice.h - rect.y-rect.h/2) / rect.h;
    float bx = 2 * (slice.x - rect.x-rect.w/2) / rect.w;
    float by = 2 * (slice.y - rect.y-rect.h/2) / rect.h;
    float tx = 2.0f * outline_thickness / rect.w;
    float ty = 2.0f * outline_thickness / rect.h;
    float cx = ax * (1.0f + tx / (1-tx));
    float cy = ay * (1.0f + ty / (1-ty));
    float dx = bx * (1.0f + tx / (1-tx));
    float dy = by * (1.0f + ty / (1-ty));
    auto i = (GLushort) m_vertex_data.size();
    m_vertex_data.push_back({x2, y1, cx, dy, ax, by});
    m_vertex_data.push_back({x2, y2, cx, cy, ax, ay});
    m_vertex_data.push_back({x1, y2, dx, cy, bx, ay});
    m_vertex_data.push_back({x1, y1, dx, dy, bx, by});
    m_elem_first.push_back(i);
    m_elem_size.push_back(4);
}


void GlRectangles::clear_rectangles()
{
    m_vertex_data.clear();
    m_elem_first.clear();
    m_elem_size.clear();
}


void GlRectangles::draw(View& view, const Vec2f& pos,
                        const Color& fill_color, const Color& outline_color,
                        float softness)
{
    init_gl_objects();

    auto program = view.impl()
            .gl_program(GlView::ProgramId::Rectangle,
                        "shaders/rectangle.vert", c_vertex_shader,
                        "shaders/rectangle.frag", c_fragment_shader);
    glUseProgram(program);
    glBindVertexArray(m_vertex_array);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // projection matrix
    GLfloat xs = 2.0f / view.scalable_size().x;
    GLfloat ys = 2.0f / view.scalable_size().y;
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
    glUniform4f(u_fill_color, fill_color.red_f(), fill_color.green_f(),
                              fill_color.blue_f(), fill_color.alpha_f());

    GLint u_outline_color = glGetUniformLocation(program, "u_outline_color");
    glUniform4f(u_outline_color, outline_color.red_f(), outline_color.green_f(),
                outline_color.blue_f(), outline_color.alpha_f());

    GLint u_softness = glGetUniformLocation(program, "u_softness");
    glUniform1f(u_softness, softness);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMultiDrawArrays(GL_TRIANGLE_FAN, m_elem_first.data(), m_elem_size.data(),
                      (GLsizei) m_elem_size.size());

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
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
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) (sizeof(GLfloat) * 2));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) (sizeof(GLfloat) * 4));

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
