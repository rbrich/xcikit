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

#include "GlPrimitives.h"
#include "GlRenderer.h"
#include "GlShader.h"
#include <cassert>

namespace xci::graphics {


constexpr GLuint c_format_narrays[] = {
    2,  // V2t2
    3,  // V2t22
    3,  // V2c4t2
    4,  // V2c4t22
};


GlPrimitives::GlPrimitives(VertexFormat format, PrimitiveType type)
        : m_format(format)
{
    assert(type == PrimitiveType::TriFans);
}


void GlPrimitives::reserve(size_t primitives, size_t vertices)
{
    m_vertex_data.reserve(vertices);
    m_elem_first.reserve(primitives);
    m_elem_size.reserve(primitives);
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


void GlPrimitives::add_vertex(ViewportCoords xy, float u, float v)
{
    assert(m_format == VertexFormat::V2t2);
    assert(m_open_vertices != -1);
    invalidate_gl_objects();
    m_vertex_data.push_back(xy.x.value);
    m_vertex_data.push_back(xy.y.value);
    m_vertex_data.push_back(u);
    m_vertex_data.push_back(v);
    m_open_vertices++;
}


void GlPrimitives::add_vertex(ViewportCoords xy, float u1, float v1, float u2, float v2)
{
    assert(m_format == VertexFormat::V2t22);
    assert(m_open_vertices != -1);
    invalidate_gl_objects();
    m_vertex_data.push_back(xy.x.value);
    m_vertex_data.push_back(xy.y.value);
    m_vertex_data.push_back(u1);
    m_vertex_data.push_back(v1);
    m_vertex_data.push_back(u2);
    m_vertex_data.push_back(v2);
    m_open_vertices++;
}


void GlPrimitives::add_vertex(ViewportCoords xy, Color c, float u, float v)
{
    assert(m_format == VertexFormat::V2c4t2);
    assert(m_open_vertices != -1);
    invalidate_gl_objects();
    m_vertex_data.push_back(xy.x.value);
    m_vertex_data.push_back(xy.y.value);
    m_vertex_data.push_back(c.red_f());
    m_vertex_data.push_back(c.green_f());
    m_vertex_data.push_back(c.blue_f());
    m_vertex_data.push_back(c.alpha_f());
    m_vertex_data.push_back(u);
    m_vertex_data.push_back(v);
    m_open_vertices++;
}


void GlPrimitives::add_vertex(ViewportCoords xy, Color c, float u1, float v1, float u2, float v2)
{
    assert(m_format == VertexFormat::V2c4t22);
    assert(m_open_vertices != -1);
    invalidate_gl_objects();
    m_vertex_data.push_back(xy.x.value);
    m_vertex_data.push_back(xy.y.value);
    m_vertex_data.push_back(c.red_f());
    m_vertex_data.push_back(c.green_f());
    m_vertex_data.push_back(c.blue_f());
    m_vertex_data.push_back(c.alpha_f());
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
    GLuint program = static_cast<GlShader*>(shader.get())->gl_program();
    glUseProgram(program);
    m_program = program;
}


void GlPrimitives::set_blend(BlendFunc func)
{
    glEnable(GL_BLEND);
    switch (func) {
        case BlendFunc::AlphaBlend:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BlendFunc::InverseVideo:
            glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
            break;
    }
}


void GlPrimitives::draw(View& view)
{
    init_gl_objects();

    glBindVertexArray(m_vertex_array);
    for (GLuint i = 0; i < c_format_narrays[int(m_format)]; i++) {
        glEnableVertexAttribArray(i);
    }

    // projection matrix
    auto mvp = view.projection_matrix();
    GLint u_mvp = glGetUniformLocation(m_program, "u_mvp");
    glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp.data());

    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    if (view.has_crop()) {
        glEnable(GL_SCISSOR_TEST);
        auto cr = view.rect_to_framebuffer(view.get_crop());
        glScissor(cr.x.as<int>(), cr.y.as<int>(), cr.w.as<int>(), cr.h.as<int>());
    }

    glMultiDrawArrays(GL_TRIANGLE_FAN, m_elem_first.data(), m_elem_size.data(),
                      (GLsizei) m_elem_size.size());

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    for (GLuint i = 0; i < c_format_narrays[int(m_format)]; i++) {
        glDisableVertexAttribArray(i);
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

    switch (m_format) {
        case VertexFormat::V2t2:
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(float) * 4,
                                  (void*) (sizeof(float) * 0));
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(float) * 4,
                                  (void*) (sizeof(float) * 2));
            break;
        case VertexFormat::V2t22:
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(float) * 6,
                                  (void*) (sizeof(float) * 0));
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(float) * 6,
                                  (void*) (sizeof(float) * 2));
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(float) * 6,
                                  (void*) (sizeof(float) * 4));
            break;
        case VertexFormat::V2c4t2:
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(float) * 8,
                                  (void*) (sizeof(float) * 0));
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
                                  sizeof(float) * 8,
                                  (void*) (sizeof(float) * 2));
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(float) * 8,
                                  (void*) (sizeof(float) * 6));
            break;
        case VertexFormat::V2c4t22:
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(float) * 10,
                                  (void*) (sizeof(float) * 0));
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
                                  sizeof(float) * 10,
                                  (void*) (sizeof(float) * 2));
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(float) * 10,
                                  (void*) (sizeof(float) * 6));
            glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(float) * 10,
                                  (void*) (sizeof(float) * 8));
            break;
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


} // namespace xci::graphics
