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

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace xci {
namespace graphics {

class GlWindow {
public:
    GlWindow();
    ~GlWindow();

    void create(const Vec2u& size, const std::string& title);
    void display(std::function<void(View& view)> draw_fn);

    View create_view();

    // access native handles
    GLFWwindow* glfw_window() { return m_window; }

private:
    GLFWwindow* m_window;
};

class Window::Impl : public GlWindow {};

}} // namespace xci::graphics

#endif // XCI_GRAPHICS_GL_WINDOW_H
