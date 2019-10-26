// GlRenderer.cpp created on 2018-04-07, part of XCI toolkit
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

#include "GlRenderer.h"
#include "GlTexture.h"
#include "GlShader.h"
#include "GlPrimitives.h"
#include <xci/core/log.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace xci::graphics {

using namespace xci::core::log;


static void glfw_error_callback(int error, const char* description)
{
    log_error("GLFW error {}: {}", error, description);
}


GlRenderer::GlRenderer(core::Vfs& vfs)
        : Renderer(vfs),
          m_file_watch(std::make_shared<core::FSDispatch>())
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        log_error("Couldn't initialize GLFW...");
    }
}


GlRenderer::~GlRenderer()
{
    glfwTerminate();
}


TexturePtr GlRenderer::create_texture()
{
    return std::make_shared<GlTexture>();
}


ShaderPtr GlRenderer::create_shader()
{
    return std::make_shared<GlShader>(m_file_watch);
}


PrimitivesPtr GlRenderer::create_primitives(VertexFormat format,
                                            PrimitiveType type)
{
    return std::make_shared<GlPrimitives>(format, type);
}


} // namespace xci::graphics
