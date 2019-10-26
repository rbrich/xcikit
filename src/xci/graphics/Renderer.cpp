// Renderer.cpp created on 2018-11-24, part of XCI toolkit
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

#include "Renderer.h"

namespace xci::graphics {


ShaderPtr Renderer::get_or_create_shader(ShaderId shader_id)
{
    if (shader_id == ShaderId::Custom)
        return create_shader();
    auto& cached_shader = m_shader[(size_t) shader_id];
    if (!cached_shader) {
        cached_shader = create_shader();
    }
    return cached_shader;
}


void Renderer::clear_shader_cache()
{
    for (auto& shader : m_shader)
        shader.reset();
}


} // namespace xci::graphics
