// VulkanWindow.h created on 2019-10-22, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_VULKAN_WINDOW_H
#define XCI_GRAPHICS_VULKAN_WINDOW_H

#include <xci/graphics/Window.h>
#include <xci/core/geometry.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace xci::graphics {

using core::Vec2i;


class VulkanWindow: public Window {
public:
    VulkanWindow();
    ~VulkanWindow() override;

    void create(const Vec2u& size, const std::string& title) override;
    void display() override;
    void wakeup() const override { glfwPostEmptyEvent(); }
    void close() const override { glfwSetWindowShouldClose(m_window, GLFW_TRUE); glfwPostEmptyEvent(); }

    void set_clipboard_string(const std::string& s) const override { glfwSetClipboardString(m_window, s.c_str()); }
    std::string get_clipboard_string() const override { return glfwGetClipboardString(m_window); }

    void set_draw_callback(DrawCallback draw_cb) override;
    void set_mouse_position_callback(MousePosCallback mpos_cb) override;
    void set_mouse_button_callback(MouseBtnCallback mbtn_cb) override;
    void set_scroll_callback(ScrollCallback scroll_cb) override;

    void set_refresh_mode(RefreshMode mode) override { m_mode = mode; }
    void set_refresh_interval(int interval) override;
    void set_refresh_timeout(std::chrono::microseconds timeout, bool periodic) override;
    void set_view_mode(ViewOrigin origin, ViewScale scale) override;

    void set_debug_flags(View::DebugFlags flags) override;

private:
    void setup_view();
    void draw();

private:
    GLFWwindow* m_window = nullptr;
    View m_view {this};
    RefreshMode m_mode = RefreshMode::OnDemand;
    Vec2i m_window_pos;
    Vec2i m_window_size;
    std::chrono::microseconds m_timeout {0};
    bool m_clear_timeout = false;
};


} // namespace xci::graphics

#endif // include guard
