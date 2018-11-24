// SfmlRenderer.h created on 2018-11-24, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_SFML_RENDERER_H
#define XCI_GRAPHICS_SFML_RENDERER_H

#include <xci/graphics/Renderer.h>

namespace xci::graphics {


class SfmlRenderer: public Renderer {
public:
    TexturePtr create_texture() override;

    ShaderPtr create_shader() override;

    PrimitivesPtr create_primitives(VertexFormat format,
                                    PrimitiveType type) override;
};


} // namespace xci::graphics

#endif // include guard
