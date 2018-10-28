// Primitives.h created on 2018-04-08, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_PRIMITIVES_H
#define XCI_GRAPHICS_PRIMITIVES_H

#include <xci/graphics/Color.h>
#include <xci/graphics/Shader.h>
#include <xci/graphics/Texture.h>
#include <xci/core/geometry.h>

namespace xci::graphics {

class View;


enum class VertexFormat {
    V2t2,       // 2 vertex coords, 2 texture coords (all float)
    V2t22,      // 2 vertex coords, 2 + 2 texture coords (all float)
    V2c4t2,     // 2 vertex coords, RGBA color, 2 texture coords (all float)
    V2c4t22,    // 2 vertex coords, RGBA color, 2 + 2 texture coords (all float)
};

enum class PrimitiveType {
    TriFans
};


class Primitives {
public:
    virtual ~Primitives() = default;

    /// Reserve memory for primitives that will be added later
    /// \param primitives   number of primitives that will be created
    /// \param vertices     total number of vertices in the primitives
    virtual void reserve(size_t primitives, size_t vertices) = 0;

    virtual void begin_primitive() = 0;
    virtual void end_primitive() = 0;
    virtual void add_vertex(float x, float y, float u, float v) = 0;
    virtual void add_vertex(float x, float y, float u1, float v1, float u2, float v2) = 0;
    virtual void add_vertex(float x, float y, Color c, float u, float v) = 0;
    virtual void add_vertex(float x, float y, Color c, float u1, float v1, float u2, float v2) = 0;
    virtual void clear() = 0;
    virtual bool empty() const = 0;

    virtual void set_shader(ShaderPtr& shader) = 0;
    virtual void set_texture(const char* name, TexturePtr& texture) = 0;

    enum class BlendFunc { AlphaBlend, InverseVideo, };
    virtual void set_blend(BlendFunc func) = 0;

    virtual void draw(View& view) = 0;
    void draw(View& view, const core::Vec2f& pos);
};


using PrimitivesPtr = std::shared_ptr<Primitives>;


} // namespace xci::graphics

#endif // XCI_GRAPHICS_PRIMITIVES_H
