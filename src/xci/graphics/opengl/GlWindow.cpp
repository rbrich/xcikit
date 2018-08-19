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
#include <xci/config.h>
#include <xci/util/log.h>
#include <xci/compat/unique.h>

#include <glad/glad.h>

#include <cassert>


namespace xci {
namespace graphics {

using namespace xci::util::log;
using namespace std::chrono;


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


void GlWindow::display()
{
    setup_view();

    auto t_last = steady_clock::now();
    while (!glfwWindowShouldClose(m_window)) {
        if (m_update_cb) {
            auto t_now = steady_clock::now();
            m_update_cb(m_view, t_now - t_last);
            t_last = t_now;
        }
        switch (m_mode) {
            case RefreshMode::OnDemand:
                if (m_view.pop_refresh())
                    draw();
                glfwWaitEvents();
                break;
            case RefreshMode::OnEvent:
                draw();
                glfwWaitEvents();
                break;
            case RefreshMode::Periodic:
                draw();
                glfwPollEvents();
                break;
        }
    }
}



void GlWindow::set_draw_callback(DrawCallback draw_cb)
{
    Window::set_draw_callback(draw_cb);
    glfwSetWindowRefreshCallback(m_window, [](GLFWwindow* window) {
        auto self = (GlWindow*) glfwGetWindowUserPointer(window);
        self->draw();
    });
}



void GlWindow::set_mouse_position_callback(Window::MousePosCallback mpos_cb)
{
    Window::set_mouse_position_callback(mpos_cb);
    glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xpos, double ypos) {
        auto self = (GlWindow*) glfwGetWindowUserPointer(window);
        if (self->m_mpos_cb) {
            auto pos = self->m_view.screen_to_scalable({(float)xpos, (float)ypos});
            self->m_mpos_cb(self->m_view, {pos});
        }
    });
}


void GlWindow::set_mouse_button_callback(MouseBtnCallback mbtn_cb)
{
    Window::set_mouse_button_callback(mbtn_cb);
    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods) {
        auto self = (GlWindow*) glfwGetWindowUserPointer(window);
        if (self->m_mbtn_cb) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            auto pos = self->m_view.screen_to_scalable({(float)xpos, (float)ypos});
            self->m_mbtn_cb(self->m_view,
                             {(MouseButton) button, (Action) action, pos});
        }
    });
}


void GlWindow::set_scroll_callback(ScrollCallback scroll_cb)
{
    Window::set_scroll_callback(scroll_cb);
    if (m_scroll_cb) {
        glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xoffset,
                                           double yoffset) {
            auto self = (GlWindow*) glfwGetWindowUserPointer(window);
            self->m_scroll_cb(self->m_view, {{float(xoffset), float(yoffset)}});
        });
    } else {
        glfwSetScrollCallback(m_window, nullptr);
    }
}


void GlWindow::set_refresh_interval(int interval)
{
    glfwSwapInterval(interval);
}


void GlWindow::set_debug_flags(View::DebugFlags flags)
{
    m_view.set_debug_flags(flags);
}


void GlWindow::setup_view()
{
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    glViewport(0, 0, width, height);
    m_view.set_framebuffer_size({(unsigned int) width, (unsigned int) height});
    glfwGetWindowSize(m_window, &width, &height);
    m_view.set_screen_size({(unsigned int) width, (unsigned int) height});
    if (m_size_cb)
        m_size_cb(m_view);

    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* win, int w, int h) {
        auto self = (GlWindow*) glfwGetWindowUserPointer(win);
        self->m_view.set_framebuffer_size({(uint) w, (uint) h});
        glViewport(0, 0, w, h);
        if (self->m_size_cb)
            self->m_size_cb(self->m_view);
    });

    glfwSetWindowSizeCallback(m_window, [](GLFWwindow* win, int w, int h) {
        auto self = (GlWindow*) glfwGetWindowUserPointer(win);
        self->m_view.set_screen_size({(uint) w, (uint) h});
        if (self->m_size_cb)
            self->m_size_cb(self->m_view);
        // Update and redraw has to be called explicitly here,
        // because glfwWaitEvents may block on resize events
        if (self->m_update_cb)
            self->m_update_cb(self->m_view, 0ns);
        self->draw();
    });

    glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode,
                                    int action, int mods)
    {
        auto self = (GlWindow*) glfwGetWindowUserPointer(window);

        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            return;
        }

        if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
            // Toggle fullscreen / windowed mode
            auto& pos = self->m_window_pos;
            auto& size = self->m_window_size;
            if (glfwGetWindowMonitor(window)) {
                glfwSetWindowMonitor(window, nullptr, pos.x, pos.y, size.x, size.y, 0);
            } else {
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                if (monitor) {
                    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                    glfwGetWindowPos(window, &pos.x, &pos.y);
                    glfwGetWindowSize(window, &size.x, &size.y);
                    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                }
            }
            return;
        }

        if (self->m_key_cb) {
            Key ev_key;
            if ((key == GLFW_KEY_SPACE)
            ||  (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
            ||  (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
            ||  (key >= GLFW_KEY_LEFT_BRACKET && key <= GLFW_KEY_RIGHT_BRACKET)) {
                ev_key = Key(key);
            } else
            if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12) {
                ev_key = Key(key - GLFW_KEY_F1 + (int)Key::F1);
            } else
            if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) {
                ev_key = Key(key - GLFW_KEY_KP_0 + (int)Key::Keypad0);
            } else {
                switch (key) {
                    case GLFW_KEY_ESCAPE: ev_key = Key::Escape; break;
                    case GLFW_KEY_ENTER: ev_key = Key::Enter; break;
                    case GLFW_KEY_BACKSPACE: ev_key = Key::Backspace; break;
                    case GLFW_KEY_TAB: ev_key = Key::Tab; break;
                    case GLFW_KEY_INSERT: ev_key = Key::Insert; break;
                    case GLFW_KEY_DELETE: ev_key = Key::Delete; break;
                    case GLFW_KEY_HOME: ev_key = Key::Home; break;
                    case GLFW_KEY_END: ev_key = Key::End; break;
                    case GLFW_KEY_PAGE_UP: ev_key = Key::PageUp; break;
                    case GLFW_KEY_PAGE_DOWN: ev_key = Key::PageDown; break;
                    case GLFW_KEY_LEFT: ev_key = Key::Left; break;
                    case GLFW_KEY_RIGHT: ev_key = Key::Right; break;
                    case GLFW_KEY_UP: ev_key = Key::Up; break;
                    case GLFW_KEY_DOWN: ev_key = Key::Down; break;
                    case GLFW_KEY_CAPS_LOCK: ev_key = Key::CapsLock; break;
                    case GLFW_KEY_SCROLL_LOCK: ev_key = Key::ScrollLock; break;
                    case GLFW_KEY_NUM_LOCK: ev_key = Key::NumLock; break;
                    case GLFW_KEY_PRINT_SCREEN: ev_key = Key::PrintScreen; break;
                    case GLFW_KEY_PAUSE: ev_key = Key::Pause; break;
                    case GLFW_KEY_SPACE: ev_key = Key::Space; break;
                    case GLFW_KEY_KP_ADD: ev_key = Key::KeypadPlus; break;
                    case GLFW_KEY_KP_SUBTRACT: ev_key = Key::KeypadMinus; break;
                    case GLFW_KEY_KP_MULTIPLY: ev_key = Key::KeypadAsterisk; break;
                    case GLFW_KEY_KP_DIVIDE: ev_key = Key::KeypadSlash; break;
                    case GLFW_KEY_KP_DECIMAL: ev_key = Key::KeypadDecimalPoint; break;
                    case GLFW_KEY_KP_ENTER: ev_key = Key::KeypadEnter; break;
                    default:
                        log_debug("GlWindow: unknown key: {}", key);
                        ev_key = Key::Unknown; break;
                }
            }

            static_assert(int(Action::Release) == GLFW_RELEASE, "GLFW_RELEASE");
            static_assert(int(Action::Press) == GLFW_PRESS, "GLFW_PRESS");
            static_assert(int(Action::Repeat) == GLFW_REPEAT, "GLFW_REPEAT");
            ModKey mod = {
                    bool(mods & GLFW_MOD_SHIFT),
                    bool(mods & GLFW_MOD_CONTROL),
                    bool(mods & GLFW_MOD_ALT),
            };
            self->m_key_cb(self->m_view, KeyEvent{ev_key, mod, Action(action)});
        }
    });

    glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int codepoint)
    {
        auto self = (GlWindow*) glfwGetWindowUserPointer(window);
        if (self->m_char_cb) {
            self->m_char_cb(self->m_view, CharEvent{codepoint});
        }
    });
}


void GlWindow::draw()
{
    glClear(GL_COLOR_BUFFER_BIT);
    if (m_draw_cb)
        m_draw_cb(m_view);
    glfwSwapBuffers(m_window);
}


}} // namespace xci::graphics
