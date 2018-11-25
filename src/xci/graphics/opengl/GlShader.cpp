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
#include "GlTexture.h"

#include <xci/config.h>
#include <xci/core/FileWatch.h>
#include <xci/core/file.h>
#include <xci/core/log.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <memory>
#include <cassert>

namespace xci::graphics {

using xci::core::read_text_file;
using xci::core::FileWatch;
using namespace xci::core::log;


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


bool GlShader::is_ready() const
{
    bool ready = m_program_ready.load(std::memory_order_acquire);
    return (ready && m_program);
}


bool GlShader::load_from_file(const std::string& vertex,
                              const std::string& fragment)
{
    m_vertex_file = vertex;
    m_fragment_file = fragment;

    clear();

    if (!reload_from_file())
        return false;

    // Force reload when shader file changes
    add_watches();

    return true;
}


bool GlShader::load_from_memory(const char* vertex_data, int vertex_size,
                                const char* fragment_data, int fragment_size)
{
    // Compile and cache new program
    m_program = compile_program(vertex_data, vertex_size,
                                fragment_data, fragment_size);
    m_program_ready.store(true, std::memory_order_release);
    return true;
}


void GlShader::set_uniform(const char* name, float f)
{
    assert(m_program != 0);
    GLint location = glGetUniformLocation(m_program, name);
    glUseProgram(m_program);
    glUniform1f(location, f);
}


void GlShader::set_uniform(const char* name,
                               float f1, float f2, float f3, float f4)
{
    assert(m_program != 0);
    GLint location = glGetUniformLocation(m_program, name);
    glUseProgram(m_program);
    glUniform4f(location, f1, f2, f3, f4);
}


void GlShader::set_texture(const char* name, TexturePtr& texture)
{
    assert(m_program != 0);

    GLint location = glGetUniformLocation(m_program, name);
    glUseProgram(m_program);
    glUniform1i(location, 0); // GL_TEXTURE0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, static_cast<GlTexture*>(texture.get())->gl_texture());
}


GLuint GlShader::gl_program()
{
    bool ok = m_program_ready.load(std::memory_order_acquire);
    if (!ok) {
        // reload
        reload_from_file();
    }
    return m_program;
}


void GlShader::add_watches()
{
    auto cb = [this](FileWatch::Event ev) {
        if (ev == FileWatch::Event::Create || ev == FileWatch::Event::Modify) {
            log_info("Shader file changed...");
            m_program_ready.store(false, std::memory_order_release);
            glfwPostEmptyEvent();
        }
    };
    // FIXME: Disable filewatches in release (NDEBUG)
    m_vertex_file_watch = m_file_watch->add_watch(m_vertex_file, cb);
    m_fragment_file_watch = m_file_watch->add_watch(m_fragment_file, cb);
    log_info("Shader watches installed");
}


void GlShader::remove_watches()
{
    if (m_vertex_file_watch != -1) {
        m_file_watch->remove_watch(m_vertex_file_watch);
        m_vertex_file_watch = -1;
    }
    if (m_fragment_file_watch != -1) {
        m_file_watch->remove_watch(m_fragment_file_watch);
        m_fragment_file_watch = -1;
    }
    log_info("Shader watches removed");
}


bool GlShader::reload_from_file()
{
    // Try to read shaders from file
    std::string vertex_file_source = read_text_file(m_vertex_file);
    std::string fragment_file_source = read_text_file(m_fragment_file);
    if (vertex_file_source.empty() || fragment_file_source.empty())
        return false;
    log_info("Loaded vertex shader: {}", m_vertex_file);
    log_info("Loaded fragment shader: {}", m_fragment_file);

    // Compile and cache new program
    m_program = compile_program(
            vertex_file_source.data(), (int) vertex_file_source.size(),
            fragment_file_source.data(), (int) fragment_file_source.size());
    m_program_ready.store(true, std::memory_order_release);
    return true;
}


void GlShader::clear()
{
    remove_watches();
    glDeleteProgram(m_program);
    m_program = 0;
    m_program_ready = false;
}


} // namespace xci::graphics
