// Shader.h created on 2018-04-08, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_SHADER_H
#define XCI_GRAPHICS_SHADER_H

#include <string>
#include <memory>

namespace xci {
namespace graphics {


enum class ShaderId {
    // Obtain one of predefined shaders
    Sprite = 0,
    SpriteC,
    Line,
    Rectangle,
    Ellipse,
    Cursor,
    // Create new, custom shader
    Custom,  // (this has to stay as last item)
};


class Shader {
public:
    virtual ~Shader() = default;

    // Load and compile GLSL program.
    virtual bool load_from_file(
            const std::string& vertex, const std::string& fragment) = 0;

    virtual bool load_from_memory(
            const char* vertex_data, int vertex_size,
            const char* fragment_data, int fragment_size) = 0;
};


using ShaderPtr = std::shared_ptr<Shader>;


}} // namespace xci::graphics


#endif // XCI_GRAPHICS_SHADER_H
