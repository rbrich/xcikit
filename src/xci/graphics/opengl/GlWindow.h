// GlWindow.h created on 2018-03-14, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_GL_WINDOW_H
#define XCI_GRAPHICS_GL_WINDOW_H

#include <xci/graphics/Window.h>
#include <xci/util/geometry.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace xci {
namespace graphics {

using util::Vec2i;

class GlWindow: public Window {
public:
    GlWindow();
    ~GlWindow() override;

    void create(const Vec2u& size, const std::string& title) override;
    void display() override;
    void wakeup() const override { glfwPostEmptyEvent(); }
    void close() const override { glfwSetWindowShouldClose(m_window, GLFW_TRUE); }

    void set_draw_callback(DrawCallback draw_cb) override;
    void set_mouse_position_callback(MousePosCallback mpos_cb) override;
    void set_mouse_button_callback(MouseBtnCallback mbtn_cb) override;
    void set_scroll_callback(ScrollCallback scroll_cb) override;

    void set_refresh_mode(RefreshMode mode) override;
    void set_debug_flags(View::DebugFlags flags) override;

    // access native handles
    GLFWwindow* glfw_window() { return m_window; }

private:
    void setup_view();
    void draw();

    GLFWwindow* m_window;
    View m_view;
    RefreshMode m_mode = RefreshMode::OnDemand;
    Vec2i m_window_pos;
    Vec2i m_window_size;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_GL_WINDOW_H
