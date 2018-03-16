// GlWindow.cpp created on 2018-03-14, part of XCI toolkit
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

#include "GlWindow.h"
#include <xci/util/log.h>

// inline
#include <xci/graphics/Window.inl>


namespace xci {
namespace graphics {

using namespace xci::util::log;


static void error_callback(int error, const char* description)
{
    log_error("GLFW error {}: {}", error, description);
}


static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}


void gl_debug_callback( GLenum source,
                      GLenum type,
                      GLuint id,
                      GLenum severity,
                      GLsizei length,
                      const GLchar* message,
                      const void* userParam )
{
    log_debug("GL (type {}, severity {}): {}",
             ( type == GL_DEBUG_TYPE_ERROR ? "ERROR" : std::to_string((int)type) ),
             severity, message );
}


GlWindow::GlWindow()
{
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        log_error("Couldn't initialize GLFW...");
        abort();
    }
}


GlWindow::~GlWindow()
{
    glfwTerminate();
}


void GlWindow::create(const Vec2u& size, const std::string& title)
{
    // OpenGL 3.3 Core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(size.x, size.y, title.c_str(),
                                nullptr, nullptr);
    if (!m_window) {
        log_error("Couldn't create GLFW window...");
        exit(1);
    }
    glfwMakeContextCurrent(m_window);
    glfwSetKeyCallback(m_window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        log_error("Couldn't initialize OpenGL...");
        exit(1);
    }
    log_info("OpenGL {} GLSL {}",
             glGetString(GL_VERSION),
             glGetString(GL_SHADING_LANGUAGE_VERSION));

#ifndef NDEBUG
    // https://www.khronos.org/opengl/wiki/OpenGL_Error
    glEnable( GL_DEBUG_OUTPUT );
    glDebugMessageCallback( (GLDEBUGPROC) gl_debug_callback, 0 );
#endif
}


void GlWindow::display(std::function<void(View& view)> draw_fn)
{
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    glViewport(0, 0, width, height);

    View view = create_view();

    while (!glfwWindowShouldClose(m_window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        draw_fn(view);
        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}


View GlWindow::create_view()
{
    View view;
    // ...
    return view;
}


}} // namespace xci::graphics
