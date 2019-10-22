// VulkanRenderer.cpp created on 2019-10-23, part of XCI toolkit
// Copyright 2019 Radek Brich
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

#include "VulkanRenderer.h"

namespace xci::graphics {


Renderer& Renderer::default_instance()
{
    static VulkanRenderer instance;
    return instance;
}


TexturePtr VulkanRenderer::create_texture()
{
    return xci::graphics::TexturePtr();
}


ShaderPtr VulkanRenderer::create_shader()
{
    return xci::graphics::ShaderPtr();
}


PrimitivesPtr VulkanRenderer::create_primitives(VertexFormat format,
                                                PrimitiveType type)
{
    return xci::graphics::PrimitivesPtr();
}


} // namespace xci::graphics
