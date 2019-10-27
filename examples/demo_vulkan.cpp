// demo_opengl.cpp created on 2018-03-14, part of XCI toolkit
// Copyright 2018, 2019 Radek Brich
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

#include <xci/graphics/vulkan/VulkanRenderer.h>
#include <xci/graphics/vulkan/VulkanWindow.h>
//#include <xci/text/Text.h>
#include <xci/core/Vfs.h>
#include <xci/config.h>

#include <iostream>
#include <cstdlib>

//using namespace xci::text;
using namespace xci::graphics;
using namespace xci::core;

int main()
{
    Vfs vfs;
    vfs.mount(XCI_SHARE_DIR);

    VulkanRenderer renderer(vfs);
    VulkanWindow window {renderer};
    window.create({800, 600}, "XCI Vulkan Demo");

    /*
    glfwMakeContextCurrent(window);

    // Setup XCI view
    View view;
    view.set_viewport_mode(ViewOrigin::Center, ViewScale::FixedScreenPixels);
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    view.set_screen_size({(float) width, (float) height});
    glfwGetFramebufferSize(window, &width, &height);
    view.set_framebuffer_size({(float) width, (float) height});
    glfwSetWindowUserPointer(window, &view);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int w, int h) {
        auto pview = (View*) glfwGetWindowUserPointer(win);
        pview->set_framebuffer_size({(float) w, (float) h});
        glViewport(0, 0, w, h);
    });
    glfwSetWindowSizeCallback(window, [](GLFWwindow* win, int w, int h) {
        auto pview = (View*) glfwGetWindowUserPointer(win);
        pview->set_screen_size({(float) w, (float) h});
    });

    // Create XCI text

    Font font;
    if (!font.add_face("fonts/ShareTechMono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;

    Text text(font, "Hello from XCI", Text::Format::None);
    text.set_font_size(50);
*/
    // Run
    while (!glfwWindowShouldClose(window.glfw_window())) {
        //glClear(GL_COLOR_BUFFER_BIT);

        //text.draw(view, {-200, 0});

        //glfwSwapBuffers(window);
        glfwWaitEvents();
    }

    //window.display();
    return EXIT_SUCCESS;
}
