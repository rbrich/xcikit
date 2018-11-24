// SfmlRenderer.cpp created on 2018-11-24, part of XCI toolkit
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

#include "SfmlRenderer.h"
#include "SfmlTexture.h"
#include "SfmlShader.h"
#include "SfmlPrimitives.h"

namespace xci::graphics {


Renderer& Renderer::default_instance()
{
    static SfmlRenderer instance;
    return instance;
}


TexturePtr SfmlRenderer::create_texture()
{
    return std::make_shared<SfmlTexture>();
}


ShaderPtr SfmlRenderer::create_shader()
{
    return std::make_shared<SfmlShader>();
}


PrimitivesPtr
SfmlRenderer::create_primitives(VertexFormat format, PrimitiveType type)
{
    return std::make_shared<SfmlPrimitives>(format, type);
}


} // namespace xci::graphics
