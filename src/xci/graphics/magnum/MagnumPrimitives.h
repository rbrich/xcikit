// MagnumPrimitives.h created on 2018-10-26, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_MAGNUM_PRIMITIVES_H
#define XCI_GRAPHICS_MAGNUM_PRIMITIVES_H

#include <xci/graphics/Primitives.h>
#include <xci/graphics/magnum/MagnumShader.h>

#include <Magnum/GL/Mesh.h>

#include <vector>

namespace xci::graphics {


class MagnumPrimitives : public Primitives {
public:
    explicit MagnumPrimitives(VertexFormat format, PrimitiveType type);
    ~MagnumPrimitives() override = default;

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
    Magnum::GL::Mesh m_mesh;

    VertexFormat m_format;
    std::vector<float> m_vertex_data;
    std::vector<GLint> m_elem_base;  // first vertex of each element
    std::vector<GLsizei> m_elem_size;  // number of vertices of each element

    int m_closed_vertices = 0;
    int m_open_vertices = -1;

    std::shared_ptr<MagnumShader> m_shader;
};


} // namespace xci::graphics

#endif // include guard
