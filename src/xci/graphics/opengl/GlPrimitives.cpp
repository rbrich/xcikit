// GlPrimitives.cpp created on 2018-04-07, part of XCI toolkit
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

#include <cassert>
#include "GlPrimitives.h"

namespace xci {
namespace graphics {


void GlPrimitives::begin_primitive()
{
    assert(m_open_vertices == -1);
    m_elem_first.push_back( (GLushort) m_vertex_data.size() );
    m_open_vertices = 0;
}


void GlPrimitives::end_primitive()
{
    assert(m_open_vertices != -1);
    m_elem_size.push_back(m_open_vertices);
    m_open_vertices = -1;
}


void GlPrimitives::add_vertex(const Vertex2Tex22& v)
{
    assert(m_open_vertices != -1);
    invalidate_gl_objects();
    m_vertex_data.push_back(v);
    m_open_vertices++;
}


void GlPrimitives::clear()
{
    m_vertex_data.clear();
    m_elem_first.clear();
    m_elem_size.clear();
}


void GlPrimitives::set_uniform(const char* name, float f)
{
    assert(m_program != 0);
    GLint location = glGetUniformLocation(m_program, name);
    glUniform1f(location, f);
}


void GlPrimitives::set_uniform(const char* name,
                               float f1, float f2, float f3, float f4)
{
    assert(m_program != 0);
    GLint location = glGetUniformLocation(m_program, name);
    glUniform4f(location, f1, f2, f3, f4);
}


void GlPrimitives::draw(View& view, const Vec2f& pos)
{
    init_gl_objects();

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

    GLint u_mvp = glGetUniformLocation(m_program, "u_mvp");
    glUniformMatrix4fv(u_mvp, 1, GL_FALSE, (const GLfloat*) mvp);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMultiDrawArrays(GL_TRIANGLE_FAN, m_elem_first.data(), m_elem_size.data(),
                      (GLsizei) m_elem_size.size());

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glUseProgram(0);
    m_program = 0;
}


void GlPrimitives::init_gl_objects()
{
    if (m_objects_ready)
        return;

    glGenVertexArrays(1, &m_vertex_array);
    glBindVertexArray(m_vertex_array);

    glGenBuffers(1, &m_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex2Tex22) * m_vertex_data.size(),
                 m_vertex_data.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex2Tex22), (void*) (sizeof(GLfloat) * 0));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex2Tex22), (void*) (sizeof(GLfloat) * 2));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex2Tex22), (void*) (sizeof(GLfloat) * 4));

    m_objects_ready = true;
}


void GlPrimitives::invalidate_gl_objects()
{
    if (!m_objects_ready)
        return;

    glDeleteVertexArrays(1, &m_vertex_array);
    glDeleteBuffers(1, &m_vertex_buffer);

    m_objects_ready = false;
}


void GlPrimitives::set_program(GLuint program)
{
    glUseProgram(program);
    m_program = program;
}


}} // namespace xci::graphics
