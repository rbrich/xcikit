// VulkanWindow.cpp created on 2019-10-22, part of XCI toolkit
// Copyright 2019 Radek Brich
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

#include "VulkanWindow.h"
#include "VulkanRenderer.h"
#include <xci/core/log.h>

namespace xci::graphics {

using namespace xci::core::log;
using namespace std::chrono;


VulkanWindow::VulkanWindow(VulkanRenderer& renderer)
    : m_renderer(renderer), m_device(m_renderer) {}


VulkanWindow::~VulkanWindow()
{
    if (m_window != nullptr)
        glfwDestroyWindow(m_window);
}


void VulkanWindow::create(const Vec2u& size, const std::string& title)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(size.x, size.y, title.c_str(),
                                nullptr, nullptr);
    if (!m_window) {
        log_error("Couldn't create GLFW window...");
        exit(1);
    }
    glfwSetWindowUserPointer(m_window, this);

    m_device.init(m_window);
}


void VulkanWindow::display()
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
            case RefreshMode::OnEvent:
                if (m_mode == RefreshMode::OnEvent || m_view.pop_refresh())
                    draw();
                if (m_timeout == 0us) {
                    glfwWaitEvents();
                } else {
                    glfwWaitEventsTimeout(double(m_timeout.count()) / 1e6);
                    if (m_clear_timeout)
                        m_timeout = 0us;
                }
                break;
            case RefreshMode::Periodic:
                draw();
                glfwPollEvents();
                break;
        }
    }
}


void VulkanWindow::set_draw_callback(Window::DrawCallback draw_cb)
{
    Window::set_draw_callback(draw_cb);
    glfwSetWindowRefreshCallback(m_window, [](GLFWwindow* window) {
        auto self = (VulkanWindow*) glfwGetWindowUserPointer(window);
        self->draw();
    });
}


void VulkanWindow::set_mouse_position_callback(Window::MousePosCallback mpos_cb)
{
    Window::set_mouse_position_callback(mpos_cb);
    glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xpos, double ypos) {
        auto self = (VulkanWindow*) glfwGetWindowUserPointer(window);
        if (self->m_mpos_cb) {
            auto pos = self->m_view.coords_to_viewport(ScreenCoords{float(xpos), float(ypos)});
            self->m_mpos_cb(self->m_view, {pos});
        }
    });
}


void VulkanWindow::set_mouse_button_callback(Window::MouseBtnCallback mbtn_cb)
{
    Window::set_mouse_button_callback(mbtn_cb);
    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods) {
        auto self = (VulkanWindow*) glfwGetWindowUserPointer(window);
        if (self->m_mbtn_cb) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            auto pos = self->m_view.coords_to_viewport(ScreenCoords{float(xpos), float(ypos)});
            self->m_mbtn_cb(self->m_view,
                            {(MouseButton) button, (Action) action, pos});
        }
    });
}


void VulkanWindow::set_scroll_callback(Window::ScrollCallback scroll_cb)
{
    Window::set_scroll_callback(scroll_cb);
    if (m_scroll_cb) {
        glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xoffset,
                                           double yoffset) {
            auto self = (VulkanWindow*) glfwGetWindowUserPointer(window);
            self->m_scroll_cb(self->m_view, {{float(xoffset), float(yoffset)}});
        });
    } else {
        glfwSetScrollCallback(m_window, nullptr);
    }
}


void VulkanWindow::set_refresh_interval(int interval)
{
    glfwSwapInterval(interval);
}


void VulkanWindow::set_refresh_timeout(std::chrono::microseconds timeout,
                                       bool periodic)
{
    m_timeout = timeout;
    m_clear_timeout = !periodic;
}


void VulkanWindow::set_view_mode(ViewOrigin origin, ViewScale scale)
{
    m_view.set_viewport_mode(origin, scale);
}


void VulkanWindow::set_debug_flags(View::DebugFlags flags)
{
    m_view.set_debug_flags(flags);
}


void VulkanWindow::setup_view()
{
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    //glViewport(0, 0, width, height);
    m_view.set_framebuffer_size({float(width), float(height)});
    glfwGetWindowSize(m_window, &width, &height);
    m_view.set_screen_size({float(width), float(height)});
    if (m_size_cb)
        m_size_cb(m_view);

    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* win, int w, int h) {
        TRACE("Framebuffer resize: {} {}", w, h);
        auto self = (VulkanWindow*) glfwGetWindowUserPointer(win);
        //glViewport(0, 0, w, h);
        if (self->m_view.set_framebuffer_size({float(w), float(h)}) && self->m_size_cb)
            self->m_size_cb(self->m_view);
    });

    glfwSetWindowSizeCallback(m_window, [](GLFWwindow* win, int w, int h) {
        TRACE("Window resize: {} {}", w, h);
        auto self = (VulkanWindow*) glfwGetWindowUserPointer(win);
        if (self->m_view.set_screen_size({float(w), float(h)}) && self->m_size_cb)
            self->m_size_cb(self->m_view);
    });

    glfwSetWindowRefreshCallback(m_window, [](GLFWwindow* win) {
        TRACE("Window refresh");
        auto self = (VulkanWindow*) glfwGetWindowUserPointer(win);
        self->draw();
    });

    glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode,
                                    int action, int mods) {
        auto self = (VulkanWindow*) glfwGetWindowUserPointer(window);

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
            // TODO
        }
    });

    glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int codepoint) {
        auto self = (VulkanWindow*) glfwGetWindowUserPointer(window);
        if (self->m_char_cb) {
            self->m_char_cb(self->m_view, CharEvent{codepoint});
        }
    });
}


void VulkanWindow::draw()
{
    TRACE("Draw");
    //glClear(GL_COLOR_BUFFER_BIT);
    if (m_draw_cb)
        m_draw_cb(m_view);
    glfwSwapBuffers(m_window);
}


Renderer& VulkanWindow::renderer()
{
    return m_renderer;
}


} // namespace xci::graphics
