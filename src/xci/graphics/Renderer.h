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
#include <xci/core/geometry.h>
#include <xci/core/Vfs.h>

#include <memory>
#include <array>
#include <cstdint>

namespace xci::graphics {


class Renderer {
public:
    explicit Renderer(core::Vfs& vfs) : m_vfs(vfs) {}
    virtual ~Renderer() = default;

    core::Vfs& vfs() { return m_vfs; }

    virtual TexturePtr create_texture() = 0;

    virtual ShaderPtr create_shader() = 0;

    virtual PrimitivesPtr create_primitives(VertexFormat format,
                                            PrimitiveType type) = 0;

    /// Create new shader or get one of the predefined shaders
    /// \param shader_id Use `Custom` to create new shader
    /// \return shared_ptr to the shader or nullptr on error
    ShaderPtr get_or_create_shader(ShaderId shader_id);

    void clear_shader_cache();

private:
    core::Vfs& m_vfs;
    static constexpr auto c_num_shaders = (size_t)ShaderId::Custom;
    std::array<ShaderPtr, c_num_shaders> m_shader = {};
};


} // namespace xci::graphics

#endif // include guard
