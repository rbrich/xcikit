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
#include <xci/util/compat.h>

// inline
#include <xci/graphics/Window.inl>


namespace xci {
namespace graphics {

using namespace xci::util::log;


static void error_callback(int error, const char* description)
{
    log_error("GLFW error {}: {}", error, description);
}


// This is enabled via CMake option
#ifdef XCI_DEBUG_OPENGL
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
#endif


// This is enabled by replacing GLAD sources with their debug version
#ifdef GLAD_DEBUG
void glad_debug_callback(const char *name, void *funcptr, int len_args, ...) {
    GLenum err_code = glad_glGetError();
    if (err_code != GL_NO_ERROR) {
        std::string err_str;
        switch (err_code) {
            case GL_INVALID_ENUM:                  err_str = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 err_str = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             err_str = "INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                err_str = "STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               err_str = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 err_str = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: err_str = "INVALID_FRAMEBUFFER_OPERATION"; break;
            default:                               err_str = "UNKNOWN_" + std::to_string(err_code); break;
        }
        log_error("GL error {} in {}", err_str, name);
    }
}
#endif


GlWindow::GlWindow()
{
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        log_error("Couldn't initialize GLFW...");
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

#ifdef XCI_DEBUG_OPENGL
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    m_window = glfwCreateWindow(size.x, size.y, title.c_str(),
                                nullptr, nullptr);
    if (!m_window) {
        log_error("Couldn't create GLFW window...");
        exit(1);
    }
    glfwMakeContextCurrent(m_window);
    glfwSetKeyCallback(m_window, key_callback);
    glfwSetWindowUserPointer(m_window, this);

    // GLAD debugging is an alternative to GL_DEBUG_OUTPUT (bellow).
    // Unlike that, it does not depend on any extension and always work.
    // To enable it, rebuild GLAD loader with `--generator="c-debug"`.
#ifdef GLAD_DEBUG
    glad_set_post_callback(glad_debug_callback);
#endif

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        log_error("Couldn't initialize OpenGL...");
        exit(1);
    }
    log_info("OpenGL {} GLSL {}",
             glGetString(GL_VERSION),
             glGetString(GL_SHADING_LANGUAGE_VERSION));

#ifdef XCI_DEBUG_OPENGL
    GLint flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        // https://www.khronos.org/opengl/wiki/Debug_Output
        // (This does not work on macOS, see GLAD debugging above instead)
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback((GLDEBUGPROC) gl_debug_callback, nullptr);
    }
#endif
}


void GlWindow::display(std::function<void(View& view)> draw_fn)
{
    setup_view();

    while (!glfwWindowShouldClose(m_window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        draw_fn(*m_view);
        glfwSwapBuffers(m_window);
        glfwWaitEvents();
    }
}


void GlWindow::set_key_callback(std::function<void(View&, KeyEvent)> key_fn)
{
    m_key_fn = std::move(key_fn);
}


void GlWindow::setup_view()
{
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    glViewport(0, 0, width, height);
    m_view = std::make_unique<View>();
    m_view->set_framebuffer_size({(unsigned int) width, (unsigned int) height});
    glfwGetWindowSize(m_window, &width, &height);
    m_view->set_screen_size({(unsigned int) width, (unsigned int) height});

    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* win, int w, int h) {
        auto self = (GlWindow*) glfwGetWindowUserPointer(win);
        self->m_view->set_framebuffer_size({(uint) w, (uint) h});
        glViewport(0, 0, w, h);
    });
    glfwSetWindowSizeCallback(m_window, [](GLFWwindow* win, int w, int h) {
        auto self = (GlWindow*) glfwGetWindowUserPointer(win);
        self->m_view->set_screen_size({(uint) w, (uint) h});
    });
}


void GlWindow::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    auto self = (GlWindow*) glfwGetWindowUserPointer(window);
    if (action == GLFW_PRESS && self->m_key_fn && self->m_view) {
        self->m_key_fn(*self->m_view, KeyEvent{Key(key)});
    }
}


}} // namespace xci::graphics
