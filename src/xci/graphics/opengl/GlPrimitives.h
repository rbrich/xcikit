// GlPrimitives.h created on 2018-04-07, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_GL_PRIMITIVES_H
#define XCI_GRAPHICS_GL_PRIMITIVES_H

#include <xci/graphics/View.h>

#include <glad/glad.h>

#include <vector>

namespace xci {
namespace graphics {


struct Vertex2Tex22 {
    float x, y;         // vertex coords
    float iu, iv;       // inner edge of the border
    float ou, ov;       // outline edge of the border
};


class GlPrimitives {
public:
    ~GlPrimitives() { invalidate_gl_objects(); }

    void begin_primitive();
    void end_primitive();
    void add_vertex(const Vertex2Tex22 &v);
    void clear();
    bool empty() const { return m_vertex_data.empty(); }

    void set_program(GLuint program);
    void set_uniform(const char* name, float f);
    void set_uniform(const char* name, float f1, float f2, float f3, float f4);
    void draw(View& view, const Vec2f& pos);

private:
    void init_gl_objects();
    void invalidate_gl_objects();

private:
    std::vector<Vertex2Tex22> m_vertex_data;
    std::vector<GLint> m_elem_first;  // first vertex of each element
    std::vector<GLsizei> m_elem_size;  // number of vertices of each element

    GLuint m_vertex_array = 0;  // aka VAO
    GLuint m_vertex_buffer = 0;  // aka VBO
    GLuint m_program = 0;
    bool m_objects_ready = false;

    int m_open_vertices = -1;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_GL_PRIMITIVES_H
