// Renderer.h created on 2018-04-08, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_RENDERER_H
#define XCI_GRAPHICS_RENDERER_H

#include "Texture.h"
#include "Shader.h"
#include "Primitives.h"
#include <xci/util/geometry.h>

#include <cstdint>
#include <memory>

namespace xci {
namespace graphics {

using xci::util::Rect_u;
using xci::util::Vec2u;
using std::uint8_t;


class Renderer {
public:
    static Renderer& default_renderer();
    virtual ~Renderer() = default;

    virtual TexturePtr new_texture() = 0;

    /// Create new shader or get one of the predefined shaders
    /// \param shader_id Use `Custom` to create new shader
    /// \return shared_ptr to the shader or nullptr on error
    virtual ShaderPtr new_shader(ShaderId shader_id) = 0;

    virtual PrimitivesPtr new_primitives(VertexFormat format,
                                         PrimitiveType type) = 0;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_RENDERER_H
