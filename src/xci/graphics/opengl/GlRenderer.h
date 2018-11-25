// GlRenderer.h created on 2018-04-07, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_GL_RENDERER_H
#define XCI_GRAPHICS_GL_RENDERER_H

#include <xci/graphics/Renderer.h>
#include <xci/core/FileWatch.h>

#include <array>


namespace xci::graphics {


class GlRenderer: public Renderer {
public:
    GlRenderer() : m_file_watch(core::FileWatch::create()) {}

    TexturePtr create_texture() override;

    ShaderPtr create_shader() override;

    PrimitivesPtr create_primitives(VertexFormat format,
                                    PrimitiveType type) override;

private:
    core::FileWatchPtr m_file_watch;
};


} // namespace xci::graphics

#endif // XCI_GRAPHICS_GL_RENDERER_H
