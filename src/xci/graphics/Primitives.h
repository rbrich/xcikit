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

#include <xci/graphics/View.h>
#include <xci/graphics/Shader.h>
#include <xci/graphics/Texture.h>

namespace xci {
namespace graphics {


enum class VertexFormat {
    V2t2,       // 2 vertex coords, 2 texture coords (all float)
    V2t22,      // 2 vertex coords, 2 + 2 texture coords (all float)
};

enum class PrimitiveType {
    TriFans
};


class Primitives {
public:
    virtual ~Primitives() = default;

    virtual void begin_primitive() = 0;
    virtual void end_primitive() = 0;
    virtual void add_vertex(float x, float y, float u, float v) = 0;
    virtual void add_vertex(float x, float y, float u1, float v1, float u2, float v2) = 0;
    virtual void clear() = 0;
    virtual bool empty() const = 0;

    virtual void set_shader(ShaderPtr& shader) = 0;
    virtual void set_uniform(const char* name, float f) = 0;
    virtual void set_uniform(const char* name, float f1, float f2, float f3, float f4) = 0;
    virtual void set_texture(const char* name, TexturePtr& texture) = 0;
    virtual void draw(View& view) = 0;

    void draw(View& view, const Vec2f& pos) {
        view.push_offset(pos);
        draw(view);
        view.pop_offset();
    }
};


using PrimitivesPtr = std::shared_ptr<Primitives>;


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_PRIMITIVES_H
