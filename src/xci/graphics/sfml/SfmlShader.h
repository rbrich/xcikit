// SfmlShader.h created on 2018-11-24, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_SFML_SHADER_H
#define XCI_GRAPHICS_SFML_SHADER_H

#include <xci/graphics/Shader.h>

#include <SFML/Graphics/Shader.hpp>

namespace xci::graphics {


class SfmlShader: public Shader {
public:
    bool is_ready() const override;

    bool load_from_file(const std::string& vertex, const std::string& fragment) override;

    bool load_from_memory(
            const char* vertex_data, int vertex_size,
            const char* fragment_data, int fragment_size) override;

    void set_uniform(const char* name, float f) override;
    void set_uniform(const char* name, float f1, float f2, float f3, float f4) override;

    void set_texture(const char* name, TexturePtr& texture) override;

    // Internal:

    sf::Shader& sfml_shader() { return m_shader; }

private:
    sf::Shader m_shader;
};


} // namespace xci::graphics

#endif // XCI_GRAPHICS_SFML_SHADER_H
