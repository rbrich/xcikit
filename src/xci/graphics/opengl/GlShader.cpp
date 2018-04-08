// GlShader.cpp created on 2018-04-08, part of XCI toolkit
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

#include "GlShader.h"

#include <xci/util/file.h>
#include <xci/util/log.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <memory>
#include <cassert>

namespace xci {
namespace graphics {

using xci::util::read_file;

using namespace xci::util::log;


GlShader::GlShader(FileWatch& fw)
        : m_file_watch(fw)
{
    reset_program();
}


GlShader::~GlShader()
{
    remove_watches();
    glDeleteProgram(program());
}


static GLuint compile_program(const char* vertex_source,
                              int vertex_source_length,
                              const char* fragment_source,
                              int fragment_source_length)
{
    // compile vertex shader
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_source, &vertex_source_length);
    glCompileShader(vertex_shader);

    // check compilation result
    GLint result = GL_FALSE;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &length);
        std::string error_message(length + 1, '\0');
        glGetShaderInfoLog(vertex_shader, length, nullptr, &error_message[0]);
        log_error("vertex shader error: {}", error_message.c_str());
        return 0;
    }

    // compile fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_source, &fragment_source_length);
    glCompileShader(fragment_shader);

    // check compilation result
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &length);
        std::string error_message(length + 1, '\0');
        glGetShaderInfoLog(fragment_shader, length, nullptr, &error_message[0]);
        log_error("fragment shader error: {}", error_message.c_str());
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    // link program
    glLinkProgram(program);

    // check link status
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::string error_message(length + 1, '\0');
        glGetProgramInfoLog(program, length, nullptr, &error_message[0]);
        log_error("shader program error: {}", error_message.c_str());
        return 0;
    }

    // cleanup
    glDetachShader(program, vertex_shader);
    glDetachShader(program, fragment_shader);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

#ifdef XCI_DEBUG_OPENGL
    // dump attributes
    GLint n, max_len;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &n);
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_len);
    for (GLuint i = 0; i < (GLuint)n; i++) {
        GLsizei length;
        GLint size;
        GLenum type;
        std::string name(max_len, '\0');
        glGetActiveAttrib(program, i, max_len, &length, &size, &type, &name[0]);
        log_debug("shader active attribute: {}", name.c_str());
    }

    // dump uniforms
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &n);
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_len);
    for (GLuint i = 0; i < (GLuint)n; i++) {
        GLsizei length;
        GLint size;
        GLenum type;
        std::string name(max_len, '\0');
        glGetActiveUniform(program, i, max_len, &length, &size, &type, &name[0]);
        log_debug("shader active uniform: {}", name.c_str());
    }
#endif

    return program;
}


bool GlShader::load_from_file(const std::string& vertex,
                              const std::string& fragment)
{
    if (!program()) {
        // Remove previous file watches, in case parameters have changed
        remove_watches();

        // Try to read shaders from file
        std::string vertex_file_source = read_file(vertex);
        std::string fragment_file_source = read_file(fragment);
        if (vertex_file_source.empty() || fragment_file_source.empty())
            return false;
        log_info("Loaded vertex shader: {}", vertex);
        log_info("Loaded fragment shader: {}", fragment);

        // Force reload when shader file changes
        add_watches(vertex, fragment);

        // Compile and cache new program
        auto program = compile_program(
                vertex_file_source.data(), (int) vertex_file_source.size(),
                fragment_file_source.data(), (int) fragment_file_source.size());
        set_program(program);
    }
    return true;
}


bool GlShader::load_from_memory(const char* vertex_data, int vertex_size,
                                const char* fragment_data, int fragment_size)
{
    if (!program()) {
        // Remove previous file watches, in case parameters have changed
        remove_watches();

        // Compile and cache new program
        auto program = compile_program(vertex_data, vertex_size,
                                       fragment_data, fragment_size);
        set_program(program);
    }
    return true;
}


void GlShader::set_program(GLuint program)
{
    m_program_atomic.store(program, std::memory_order_release);
}


GLuint GlShader::program() const
{
    return m_program_atomic.load(std::memory_order_acquire);
}


void GlShader::add_watches(const std::string& vertex, const std::string& fragment)
{
    auto& prog = m_program_atomic;
    auto cb = [&prog](FileWatch::Event) {
        prog.store(0, std::memory_order_release);
        glfwPostEmptyEvent();
    };
    m_vertex_file_watch = m_file_watch.add_watch(vertex, cb);
    m_fragment_file_watch = m_file_watch.add_watch(fragment, cb);
}


void GlShader::remove_watches()
{
    if (m_vertex_file_watch != -1) {
        m_file_watch.remove_watch(m_vertex_file_watch);
        m_vertex_file_watch = -1;
    }
    if (m_fragment_file_watch != -1) {
        m_file_watch.remove_watch(m_fragment_file_watch);
        m_fragment_file_watch = -1;
    }
}


}} // namespace xci::graphics
