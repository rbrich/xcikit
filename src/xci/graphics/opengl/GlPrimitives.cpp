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
#include "GlRenderer.h"
#include "GlTexture.h"
#include "GlShader.h"

namespace xci {
namespace graphics {


GlPrimitives::GlPrimitives(VertexFormat format, PrimitiveType type)
        : m_format(format)
{
    assert(type == PrimitiveType::TriFans);
}


void GlPrimitives::begin_primitive()
{
    assert(m_open_vertices == -1);
    m_elem_first.push_back(m_closed_vertices);
    m_open_vertices = 0;
}


void GlPrimitives::end_primitive()
{
    assert(m_open_vertices != -1);
    m_elem_size.push_back(m_open_vertices);
    m_closed_vertices += m_open_vertices;
    m_open_vertices = -1;
}


void GlPrimitives::add_vertex(float x, float y, float u, float v)
{
    assert(m_format == VertexFormat::V2t2);
    assert(m_open_vertices != -1);
    invalidate_gl_objects();
    m_vertex_data.push_back(x);
    m_vertex_data.push_back(y);
    m_vertex_data.push_back(u);
    m_vertex_data.push_back(v);
    m_open_vertices++;
}


void GlPrimitives::add_vertex(float x, float y, float u1, float v1, float u2, float v2)
{
    assert(m_format == VertexFormat::V2t22);
    assert(m_open_vertices != -1);
    invalidate_gl_objects();
    m_vertex_data.push_back(x);
    m_vertex_data.push_back(y);
    m_vertex_data.push_back(u1);
    m_vertex_data.push_back(v1);
    m_vertex_data.push_back(u2);
    m_vertex_data.push_back(v2);
    m_open_vertices++;
}


void GlPrimitives::clear()
{
    m_vertex_data.clear();
    m_elem_first.clear();
    m_elem_size.clear();
    m_closed_vertices = 0;
    m_open_vertices = -1;
    m_objects_ready = false;
}


void GlPrimitives::set_shader(ShaderPtr& shader)
{
    GLuint program = static_cast<GlShader*>(shader.get())->program();
    glUseProgram(program);
    m_program = program;
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


void GlPrimitives::set_texture(const char* name, TexturePtr& texture)
{
    assert(m_program != 0);

    GLint location = glGetUniformLocation(m_program, name);
    glUniform1i(location, 0); // GL_TEXTURE0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, static_cast<GlTexture*>(texture.get())->gl_texture());
}


void GlPrimitives::draw(View& view, const Vec2f& pos)
{
    init_gl_objects();

    glBindVertexArray(m_vertex_array);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    if (m_format == VertexFormat::V2t22) {
        glEnableVertexAttribArray(2);
    }

    // projection matrix
    GLfloat xs = 2.0f / view.scalable_size().x;
    GLfloat ys = 2.0f / view.scalable_size().y;
    GLfloat xt = pos.x * xs;
    GLfloat yt = pos.y * ys;
    const GLfloat mvp[] = {
            xs,   0.0f, 0.0f, 0.0f,
            0.0f, -ys,  0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            xt,   -yt,  0.0f, 1.0f,
    };

    GLint u_mvp = glGetUniformLocation(m_program, "u_mvp");
    glUniformMatrix4fv(u_mvp, 1, GL_FALSE, (const GLfloat*) mvp);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    glMultiDrawArrays(GL_TRIANGLE_FAN, m_elem_first.data(), m_elem_size.data(),
                      (GLsizei) m_elem_size.size());

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    if (m_format == VertexFormat::V2t22) {
        glDisableVertexAttribArray(2);
    }
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * m_vertex_data.size(),
                 m_vertex_data.data(), GL_STATIC_DRAW);

    if (m_format == VertexFormat::V2t22) {
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                              sizeof(float) * 6, (void*) (sizeof(float) * 0));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                              sizeof(float) * 6, (void*) (sizeof(float) * 2));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                              sizeof(float) * 6, (void*) (sizeof(float) * 4));
    }
    if (m_format == VertexFormat::V2t2) {
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                              sizeof(float) * 4, (void*) (sizeof(float) * 0));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                              sizeof(float) * 4, (void*) (sizeof(float) * 2));
    }

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


}} // namespace xci::graphics
