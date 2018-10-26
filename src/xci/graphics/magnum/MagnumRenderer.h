// MagnumRenderer.h created on 2018-10-26, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_MAGNUM_RENDERER_H
#define XCI_GRAPHICS_MAGNUM_RENDERER_H

#include <xci/graphics/Renderer.h>

namespace xci::graphics {


class MagnumRenderer: public Renderer {
public:

    TexturePtr create_texture() override;

    ShaderPtr get_or_create_shader(ShaderId shader_id) override;

    PrimitivesPtr create_primitives(VertexFormat format,
                                    PrimitiveType type) override;

private:
    static constexpr auto c_num_shaders = (size_t)ShaderId::Custom;
    std::array<ShaderPtr, c_num_shaders> m_shader = {};
};


} // namespace xci::graphics

#endif // include guard
