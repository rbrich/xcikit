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

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace xci {
namespace graphics {


class GlWindow: public Window {
public:
    GlWindow();
    ~GlWindow() override;

    void create(const Vec2u& size, const std::string& title) override;
    void display() override;

    void set_size_callback(std::function<void(View&)> size_cb) override;
    void set_draw_callback(std::function<void(View&)> draw_cb) override;
    void set_key_callback(std::function<void(View&, KeyEvent)> key_cb) override;
    void set_mouse_position_callback(MousePosCallback mpos_cb) override;
    void set_mouse_button_callback(MouseBtnCallback mbtn_cb) override;

    void set_refresh_mode(RefreshMode mode) override;

    // access native handles
    GLFWwindow* glfw_window() { return m_window; }

private:
    void setup_view();
    void draw();

    GLFWwindow* m_window;
    std::unique_ptr<View> m_view;
    std::function<void(View&)> m_size_cb;
    std::function<void(View&)> m_draw_cb;
    std::function<void(View&, KeyEvent)> m_key_cb;
    MousePosCallback m_mpos_cb;
    MouseBtnCallback m_mbtn_cb;
    RefreshMode m_mode = RefreshMode::OnDemand;
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_GL_WINDOW_H
