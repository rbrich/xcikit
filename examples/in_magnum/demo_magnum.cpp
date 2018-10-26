// demo_magnum.cpp created on 2018-10-25, part of XCI toolkit
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
#include <xci/core/Vfs.h>
#include <xci/config.h>

#include <Magnum/Magnum.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Platform/GLContext.h>
#include <Magnum/Shaders/VertexColor.h>

#include <GLFW/glfw3.h>

#include <cstdlib>

using namespace xci::text;
using namespace xci::graphics;
using namespace xci::core;
using namespace Magnum;
using namespace Magnum::Math::Literals;

int main(int argc, char** argv) {
    // XCI vfs
    auto& vfs = Vfs::default_instance();
    vfs.mount_dir(XCI_SHARE_DIR);

    // GLFW window
    if (!glfwInit())
        return EXIT_FAILURE;

    // OpenGL 3.3 Core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* const window = glfwCreateWindow(800, 600, "XCI Magnum Demo", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);

    // Magnum
    Platform::GLContext ctx{argc, argv};

    /* Setup the colored triangle */
    struct TriangleVertex {
        Vector2 position;
        Color3 color;
    };
    const TriangleVertex data[]{
            {{-0.5f, -0.5f}, 0xff0000_rgbf},    /* Left vertex, red color */
            {{ 0.5f, -0.5f}, 0x00ff00_rgbf},    /* Right vertex, green color */
            {{ 0.0f,  0.5f}, 0x0000ff_rgbf}     /* Top vertex, blue color */
    };

    GL::Buffer buffer;
    buffer.setData(data);

    GL::Mesh mesh;
    mesh.setPrimitive(GL::MeshPrimitive::Triangles)
            .setCount(3)
            .addVertexBuffer(buffer, 0,
                             Shaders::VertexColor2D::Position{},
                             Shaders::VertexColor2D::Color3{});

    Shaders::VertexColor2D shader;

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
    Font font;
    {
        auto face_file = vfs.open("fonts/ShareTechMono/ShareTechMono-Regular.ttf");
        auto face = FontLibrary::default_instance()->create_font_face();
        if (!face->load_from_file(face_file.path(), 0))
            return EXIT_FAILURE;
        font.add_face(std::move(face));
    }
    Text text("Hello from XCI", font);
    text.set_size(0.2);

    /* Loop until the user closes the window */
    while(!glfwWindowShouldClose(window)) {

        /* Render here */
        GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);
        mesh.draw(shader);

        text.resize_draw(view, {-1.0f, -0.333f});

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
}
