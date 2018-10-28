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

namespace xci::graphics {


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

    /// Is this shader already loaded?
    virtual bool is_ready() const = 0;

    // Load and compile GLSL program:

    /// Load program from VFS
    /// This in turn calls either `load_from_file` or `load_from_memory`
    /// depending on kind of VfsLoader used (real file or archive)
    bool load_from_vfs(const std::string& vertex, const std::string& fragment);

    /// Load program from a file (possibly creating FileWatch for auto-reload)
    virtual bool load_from_file(
            const std::string& vertex, const std::string& fragment) = 0;

    /// Load program directly from memory
    virtual bool load_from_memory(
            const char* vertex_data, int vertex_size,
            const char* fragment_data, int fragment_size) = 0;

    virtual void set_uniform(const char* name, float f) = 0;
    virtual void set_uniform(const char* name, float f1, float f2, float f3, float f4) = 0;
};


using ShaderPtr = std::shared_ptr<Shader>;


} // namespace xci::graphics

#endif // XCI_GRAPHICS_SHADER_H
