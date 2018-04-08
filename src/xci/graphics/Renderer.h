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

#include "Shader.h"
#include "Texture.h"
#include <xci/util/geometry.h>

#include <cstdint>
#include <memory>

namespace xci {
namespace graphics {

using xci::util::Rect_u;
using xci::util::Vec2u;
using std::uint8_t;

using ShaderPtr = std::shared_ptr<Shader>;
using TexturePtr = std::shared_ptr<Texture>;


class Renderer {
public:
    static Renderer& default_renderer();
    virtual ~Renderer();

    // ------------------------------------------------------------------------
    // Shader

    enum class ShaderId {
        // Obtain one of predefined shaders
        Sprite = 0,
        Rectangle,
        Ellipse,
        // Create new, custom shader
        Custom,  // (this has to stay as last item)
    };

    // Create new shader or get one of the predefined shaders
    virtual ShaderPtr new_shader(ShaderId shader_id) = 0;

    // ------------------------------------------------------------------------
    // Texture

    virtual TexturePtr new_texture() = 0;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_RENDERER_H
