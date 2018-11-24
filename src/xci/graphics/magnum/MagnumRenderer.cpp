// MagnumRenderer.cpp created on 2018-10-26, part of XCI toolkit
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

#include "MagnumRenderer.h"
#include "MagnumTexture.h"
#include "MagnumShader.h"
#include "MagnumPrimitives.h"

namespace xci::graphics {


Renderer& Renderer::default_instance()
{
    static MagnumRenderer instance;
    return instance;
}


TexturePtr MagnumRenderer::create_texture()
{
    return std::make_shared<MagnumTexture>();
}


ShaderPtr MagnumRenderer::create_shader()
{
    return std::make_shared<MagnumShader>();
}


PrimitivesPtr
MagnumRenderer::create_primitives(VertexFormat format, PrimitiveType type)
{
    return std::make_shared<MagnumPrimitives>(format, type);
}


} // namespace xci::graphics
