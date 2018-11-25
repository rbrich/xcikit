// SfmlPrimitives.h created on 2018-03-04, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_SFML_PRIMITIVES_H
#define XCI_GRAPHICS_SFML_PRIMITIVES_H

#include <xci/graphics/Primitives.h>
#include <xci/graphics/sfml/SfmlTexture.h>

#include <glad/glad.h>

namespace xci::graphics {


class SfmlPrimitives: public Primitives {
public:
    explicit SfmlPrimitives(VertexFormat format, PrimitiveType type);
    ~SfmlPrimitives() override { invalidate_gl_objects(); }

    void reserve(size_t primitives, size_t vertices) override;

    void begin_primitive() override;
    void end_primitive() override;
    void add_vertex(float x, float y, float u, float v) override;
    void add_vertex(float x, float y, float u1, float v1, float u2, float v2) override;
    void add_vertex(float x, float y, Color c, float u, float v) override;
    void add_vertex(float x, float y, Color c, float u1, float v1, float u2, float v2) override;
    void clear() override;
    bool empty() const override { return m_vertex_data.empty(); }

    void set_shader(ShaderPtr& shader) override;
    void set_blend(BlendFunc func) override;

    void draw(View& view) override;

private:
    void init_gl_objects();
    void invalidate_gl_objects();

private:
    // Note: SFML 2.5 has VertexBuffer, but we can't use that,
    // because it forces vertex format to coord/color/texcoord,
    // which is incompatible with most of our shaders.

    VertexFormat m_format;
    std::vector<float> m_vertex_data;
    std::vector<GLint> m_elem_first;  // first vertex of each element
    std::vector<GLsizei> m_elem_size;  // number of vertices of each element

    GLuint m_vertex_array = 0;  // aka VAO
    GLuint m_vertex_buffer = 0;  // aka VBO
    GLuint m_program = 0;
    bool m_objects_ready = false;

    int m_closed_vertices = 0;
    int m_open_vertices = -1;
};


} // namespace xci::graphics

#endif // include guard
