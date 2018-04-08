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
#include <xci/graphics/Renderer.h>
#include <xci/graphics/Texture.h>

namespace xci {
namespace graphics {


class Primitives {
public:
    enum class VertexFormat {
        V2T2,
        V2T22,
    };

    explicit Primitives(VertexFormat format);
    ~Primitives();
    Primitives(Primitives&&) noexcept;
    Primitives& operator=(Primitives&&) noexcept;

    void begin_primitive();
    void end_primitive();
    void add_vertex(float x, float y, float u, float v);
    void add_vertex(float x, float y, float u1, float v1, float u2, float v2);
    void clear();
    bool empty() const;

    void set_shader(ShaderPtr& shader);
    void set_uniform(const char* name, float f);
    void set_uniform(const char* name, float f1, float f2, float f3, float f4);
    void set_texture(const char* name, const Texture& texture);
    void draw(View& view, const Vec2f& pos);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_PRIMITIVES_H
