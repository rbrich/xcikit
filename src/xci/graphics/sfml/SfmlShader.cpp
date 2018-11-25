// SfmlShader.cpp created on 2018-11-24, part of XCI toolkit
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

#include "SfmlShader.h"
#include "SfmlTexture.h"

#include <SFML/Graphics/Glsl.hpp>

namespace xci::graphics {


bool SfmlShader::is_ready() const
{
    return m_shader.getNativeHandle() != 0;
}


bool SfmlShader::load_from_file(const std::string& vertex,
                                const std::string& fragment)
{
    return m_shader.loadFromFile(vertex, fragment);
}


bool SfmlShader::load_from_memory(const char* vertex_data, int vertex_size,
                                  const char* fragment_data, int fragment_size)
{
    return m_shader.loadFromMemory({vertex_data, size_t(vertex_size)},
                                   {fragment_data, size_t(fragment_size)});
}


void SfmlShader::set_uniform(const char* name, float f)
{
    m_shader.setUniform(name, f);
}


void SfmlShader::set_uniform(const char* name,
                             float f1, float f2, float f3, float f4)
{
    m_shader.setUniform(name, sf::Glsl::Vec4{f1, f2, f3, f4});
}


void SfmlShader::set_texture(const char* name, TexturePtr& texture)
{
    auto& sfml_texture = static_cast<SfmlTexture*>(texture.get())->sfml_texture();
    m_shader.setUniform(name, sfml_texture);
    sf::Texture::bind(&sfml_texture);
}


} // namespace xci::graphics
