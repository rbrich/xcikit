// MagnumPrimitives.cpp created on 2018-10-26, part of XCI toolkit
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

#include "MagnumPrimitives.h"
#include "MagnumShader.h"

#include <cassert>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/MeshView.h>
#include <Magnum/GL/AbstractShaderProgram.h>

namespace xci::graphics {

using namespace Magnum;


MagnumPrimitives::MagnumPrimitives(VertexFormat format, PrimitiveType type)
        : m_format(format)
{
    assert(type == PrimitiveType::TriFans);
    m_mesh.setPrimitive(GL::MeshPrimitive::TriangleFan);
}


void MagnumPrimitives::reserve(size_t primitives, size_t vertices)
{
    m_vertex_data.reserve(vertices);
    m_elem_base.reserve(primitives);
    m_elem_size.reserve(primitives);
}


void MagnumPrimitives::begin_primitive()
{
    assert(m_open_vertices == -1);
    m_elem_base.push_back(m_closed_vertices);
    m_open_vertices = 0;
}


void MagnumPrimitives::end_primitive()
{
    assert(m_open_vertices != -1);
    m_elem_size.push_back(m_open_vertices);
    m_closed_vertices += m_open_vertices;
    m_open_vertices = -1;
}


void MagnumPrimitives::add_vertex(float x, float y, float u, float v)
{
    assert(m_format == VertexFormat::V2t2);
    assert(m_open_vertices != -1);
    m_vertex_data.push_back(x);
    m_vertex_data.push_back(y);
    m_vertex_data.push_back(u);
    m_vertex_data.push_back(v);
    m_open_vertices++;
}


void
MagnumPrimitives::add_vertex(float x, float y, float u1, float v1, float u2,
                             float v2)
{
    assert(m_format == VertexFormat::V2t22);
    assert(m_open_vertices != -1);
    m_vertex_data.push_back(x);
    m_vertex_data.push_back(y);
    m_vertex_data.push_back(u1);
    m_vertex_data.push_back(v1);
    m_vertex_data.push_back(u2);
    m_vertex_data.push_back(v2);
    m_open_vertices++;
}


void MagnumPrimitives::add_vertex(float x, float y, Color c, float u, float v)
{
    assert(m_format == VertexFormat::V2c4t2);
    assert(m_open_vertices != -1);
    m_vertex_data.push_back(x);
    m_vertex_data.push_back(y);
    m_vertex_data.push_back(c.red_f());
    m_vertex_data.push_back(c.green_f());
    m_vertex_data.push_back(c.blue_f());
    m_vertex_data.push_back(c.alpha_f());
    m_vertex_data.push_back(u);
    m_vertex_data.push_back(v);
    m_open_vertices++;
}


void MagnumPrimitives::add_vertex(float x, float y, Color c, float u1, float v1,
                                  float u2, float v2)
{
    assert(m_format == VertexFormat::V2c4t22);
    assert(m_open_vertices != -1);
    m_vertex_data.push_back(x);
    m_vertex_data.push_back(y);
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


void MagnumPrimitives::clear()
{
    m_vertex_data.clear();
    m_elem_base.clear();
    m_elem_size.clear();
    m_closed_vertices = 0;
    m_open_vertices = -1;
}


void MagnumPrimitives::set_shader(ShaderPtr& shader)
{
    m_shader = std::static_pointer_cast<MagnumShader>(shader);
}


void MagnumPrimitives::set_texture(const char* name, TexturePtr& texture)
{

}


void MagnumPrimitives::set_blend(Primitives::BlendFunc func)
{

}


class ShaderV2t2: public GL::AbstractShaderProgram {
public:
    typedef GL::Attribute<0, Vector2> Position;
    typedef GL::Attribute<1, Vector2> TexCoords;
};


class ShaderV2t22: public GL::AbstractShaderProgram {
public:
    typedef GL::Attribute<0, Vector2> Position;
    typedef GL::Attribute<1, Vector2> TexCoords1;
    typedef GL::Attribute<2, Vector2> TexCoords2;
};


class ShaderV2c4t2: public GL::AbstractShaderProgram {
public:
    typedef GL::Attribute<0, Vector2> Position;
    typedef GL::Attribute<1, Color4> Color;
    typedef GL::Attribute<2, Vector2> TexCoords;
};


class ShaderV2c4t22: public GL::AbstractShaderProgram {
public:
    typedef GL::Attribute<0, Vector2> Position;
    typedef GL::Attribute<1, Color4> Color;
    typedef GL::Attribute<2, Vector2> TexCoords1;
    typedef GL::Attribute<3, Vector2> TexCoords2;
};


void MagnumPrimitives::draw(View& view)
{
    GL::Buffer vertex_buffer;
    vertex_buffer.setData(m_vertex_data, GL::BufferUsage::StaticDraw);

    switch (m_format) {
        case VertexFormat::V2t2:
            m_mesh.setCount(static_cast<Magnum::Int>(m_elem_base.size()))
                  .addVertexBuffer(vertex_buffer, 0,
                          ShaderV2t2::Position{},
                          ShaderV2t2::TexCoords{});
            break;
        case VertexFormat::V2t22:
            m_mesh.setCount(static_cast<Magnum::Int>(m_elem_base.size()))
                    .addVertexBuffer(vertex_buffer, 0,
                                     ShaderV2t22::Position{},
                                     ShaderV2t22::TexCoords1{},
                                     ShaderV2t22::TexCoords2{});
            break;
        case VertexFormat::V2c4t2:
            m_mesh.setCount(static_cast<Magnum::Int>(m_elem_base.size()))
                    .addVertexBuffer(vertex_buffer, 0,
                                     ShaderV2c4t2::Position{},
                                     ShaderV2c4t2::Color{},
                                     ShaderV2c4t2::TexCoords{});
            break;
        case VertexFormat::V2c4t22:
            m_mesh.setCount(static_cast<Magnum::Int>(m_elem_base.size()))
                    .addVertexBuffer(vertex_buffer, 0,
                                     ShaderV2c4t22::Position{},
                                     ShaderV2c4t22::Color{},
                                     ShaderV2c4t22::TexCoords1{},
                                     ShaderV2c4t22::TexCoords2{});
            break;
    }

    auto it_base = m_elem_base.cbegin();
    auto it_size = m_elem_size.cbegin();
    for (; (it_base != m_elem_base.cend()) && (it_size != m_elem_size.cend());
            ++it_base, ++it_size) {
        MeshView(m_mesh)
            .setBaseVertex(*it_base)
            .setCount(*it_size)
            .draw(m_shader->program());
    }

}


} // namespace xci::graphics
