// GlView.cpp created on 2018-03-14, part of XCI toolkit
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

#include "GlView.h"
#include <xci/util/log.h>

// inline
#include <xci/graphics/View.inl>

namespace xci {
namespace graphics {


using namespace xci::util::log;


static const char* c_vertex_shader = R"~~~(
#version 330

uniform mat4 u_mvp;

in vec2 a_position;
in vec4 a_color;
in vec2 a_tex_coord;

out vec4 v_color;
out vec2 v_tex_coord;

void main() {
    gl_Position = u_mvp * vec4(a_position, 0.0, 1.0);
    v_tex_coord = a_tex_coord;
    v_color = a_color;
}
)~~~";

static const char* c_fragment_shader = R"~~~(
#version 330

uniform sampler2D u_texture;

in vec4 v_color;
in vec2 v_tex_coord;

out vec4 o_color;

void main() {
    float alpha = texture(u_texture, v_tex_coord).r;
    o_color = v_color * vec4(1.0, 1.0, 1.0, alpha);
}
)~~~";


GlView::GlView(Vec2u pixel_size)
    : m_pixel_size(pixel_size)
{
    resize(pixel_size);

    // compile vertex shader
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &c_vertex_shader, nullptr);
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
        exit(1);
    }

    // compile fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &c_fragment_shader, nullptr);
    glCompileShader(fragment_shader);

    // check compilation result
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &length);
        std::string error_message(length + 1, '\0');
        glGetShaderInfoLog(fragment_shader, length, nullptr, &error_message[0]);
        log_error("fragment shader error: {}", error_message.c_str());
        exit(1);
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, vertex_shader);
    glAttachShader(m_program, fragment_shader);

    // fixed attribute locations for VBO
    glBindAttribLocation(m_program, 0, "a_position");
    glBindAttribLocation(m_program, 1, "a_color");
    glBindAttribLocation(m_program, 2, "a_tex_coord");

    // link program
    glLinkProgram(m_program);

    // check link status
    glGetProgramiv(m_program, GL_LINK_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &length);
        std::string error_message(length + 1, '\0');
        glGetProgramInfoLog(m_program, length, nullptr, &error_message[0]);
        log_error("shader program error: {}", error_message.c_str());
        exit(1);
    }

    // cleanup
    glDetachShader(m_program, vertex_shader);
    glDetachShader(m_program, fragment_shader);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

#ifndef NDEBUG
    // dump attributes
    GLint n, max_len;
    glGetProgramiv(m_program, GL_ACTIVE_ATTRIBUTES, &n);
    glGetProgramiv(m_program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_len);
    for (GLuint i = 0; i < (GLuint)n; i++) {
        GLsizei length;
        GLint size;
        GLenum type;
        std::string name(max_len, '\0');
        glGetActiveAttrib(m_program, i, max_len, &length, &size, &type, &name[0]);
        log_debug("shader active attribute: {}", name.c_str());
    }

    // dump uniforms
    glGetProgramiv(m_program, GL_ACTIVE_UNIFORMS, &n);
    glGetProgramiv(m_program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_len);
    for (GLuint i = 0; i < (GLuint)n; i++) {
        GLsizei length;
        GLint size;
        GLenum type;
        std::string name(max_len, '\0');
        glGetActiveUniform(m_program, i, max_len, &length, &size, &type, &name[0]);
        log_debug("shader active uniform: {}", name.c_str());
    }
#endif
}


GlView::~GlView()
{
    glDeleteProgram(m_program);
}


void GlView::resize(Vec2u pixel_size)
{
    // Decide between vert+/hor+ depending on screen orientation.
    if (pixel_size.x < pixel_size.y) {
        // preserve screen width
        float aspect = float(pixel_size.y) / float(pixel_size.x);
        m_size = {2.0f, 2.0f * aspect};
    } else {
        // preserve screen height
        float aspect = float(pixel_size.x) / float(pixel_size.y);
        m_size = {2.0f * aspect, 2.0f};
    }

    m_pixel_size = pixel_size;
}


}} // namespace xci::graphics
