// demo_sfml.cc - created by Radek Brich on 2018-11-25
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
#include <xci/core/log.h>
#include <xci/config.h>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Window.hpp>
#include <SFML/Window/Event.hpp>

#include <glad/glad.h>

#include <cstdlib>

using namespace xci::text;
using namespace xci::graphics;
using namespace xci::core;
using namespace xci::core::log;

int main()
{
    auto& vfs = Vfs::default_instance();
    vfs.mount(XCI_SHARE_DIR);

    // === Create SFML window ===

    // OpenGL 3.3 Core profile
    sf::ContextSettings settings;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.attributeFlags = sf::ContextSettings::Core;

    sf::RenderWindow window;
    unsigned w = 800, h = 600;
    window.create({w, h}, "XCI SFML Demo", sf::Style::Default, settings);
    {
        sf::View view;
        view.setCenter(0, 0);
        view.setSize(w, h);
        window.setView(view);
    }
    window.setActive(true);

    // === Setup GLAD ===

    if (!gladLoadGL()) {
        log_error("Couldn't initialize OpenGL...");
        return EXIT_FAILURE;
    }
    log_info("OpenGL {} GLSL {}",
             glGetString(GL_VERSION),
             glGetString(GL_SHADING_LANGUAGE_VERSION));

    // === XCI ===

    // Setup XCI view
    View view;
    auto wsize = window.getSize();
    view.set_framebuffer_size({wsize.x, wsize.y});
    view.set_screen_size({wsize.x, wsize.y});
    glViewport(0, 0, wsize.x, wsize.y);

    // Create XCI text
    Font font;
    {
        auto face_file = vfs.read_file("fonts/ShareTechMono/ShareTechMono-Regular.ttf");
        auto face = FontLibrary::default_instance()->create_font_face();
        if (!face->load_from_file(face_file.path(), 0))
            return EXIT_FAILURE;
        font.add_face(std::move(face));
    }
    Text text("Hello from XCI", font);
    text.set_size(0.2);

    while (window.isOpen()) {
        sf::Event event = {};
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::Resized:
                    view.set_framebuffer_size({event.size.width, event.size.height});
                    view.set_screen_size({event.size.width, event.size.height});
                    glViewport(0, 0, event.size.width, event.size.height);
                    break;
                default:
                    break;
            }
        }

        window.clear();

        text.resize_draw(view, {-1.0f, -0.333f});

        window.display();
    }
    return EXIT_SUCCESS;
}
