// demo_opengl.cpp created on 2018-03-14, part of XCI toolkit
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

#include <xci/text/Text.h>
#include <xci/util/file.h>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdlib>

using namespace xci::text;
using namespace xci::graphics;

int main()
{
    xci::util::chdir_to_share();

    if (!glfwInit())
        return EXIT_FAILURE;

    // OpenGL 3.3 Core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Setup GLFW window
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "XCI OpenGL Demo", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);

    // Setup GLAD loader
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        return EXIT_FAILURE;
    }

    // Setup XCI view
    View view;
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    view.set_screen_size({(unsigned) width, (unsigned) height});
    glfwGetFramebufferSize(window, &width, &height);
    view.set_framebuffer_size({(unsigned) width, (unsigned) height});
    glfwSetWindowUserPointer(window, &view);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int w, int h) {
        auto pview = (View*) glfwGetWindowUserPointer(win);
        pview->set_framebuffer_size({(unsigned) w, (unsigned) h});
        glViewport(0, 0, w, h);
    });

    // Create XCI text
    auto face = std::make_unique<FontFace>();
    if (!face->load_from_file("fonts/ShareTechMono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;
    Font font;
    font.add_face(std::move(face));
    Text text("Hello from XCI", font);
    text.set_size(0.2);

    // Run
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        text.resize_draw(view, {-1.0f, -0.333f});

        glfwSwapBuffers(window);
        glfwWaitEvents();
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}
