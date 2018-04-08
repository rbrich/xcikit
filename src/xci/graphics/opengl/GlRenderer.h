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
#include <xci/util/FileWatch.h>

#include <glad/glad.h>

#include <array>
#include <atomic>

namespace xci {
namespace graphics {

using xci::util::FileWatch;


class GlRenderer: public Renderer {
public:
    // ------------------------------------------------------------------------
    // Shaders

    ShaderPtr new_shader(ShaderId shader_id) override;

    // If successful, setup a watch on the file to auto-reload on any change.
    bool shader_load_from_file(ShaderPtr& shader,
            const std::string& vertex, const std::string& fragment) override;

    bool shader_load_from_memory(ShaderPtr& shader,
            const char* vertex_data, int vertex_size,
            const char* fragment_data, int fragment_size) override;

private:
    FileWatch m_file_watch;
    static constexpr auto c_num_shaders = (size_t)ShaderId::Custom;
    std::array<ShaderPtr, c_num_shaders> m_shader = {};
};


class Shader {
public:
    explicit Shader(FileWatch& fw);
    ~Shader();

    GLuint program() const;
    void set_program(GLuint program);
    void reset_program() { set_program(0); }

    void add_watches(const std::string& vertex,
                     const std::string& fragment);
    void remove_watches();

private:
    FileWatch& m_file_watch;
    std::atomic<GLuint> m_program_atomic;
    int m_vertex_file_watch = -1;
    int m_fragment_file_watch = -1;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_GL_RENDERER_H
