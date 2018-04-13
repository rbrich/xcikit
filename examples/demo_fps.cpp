// demo_fps.cpp created on 2018-04-14, part of XCI toolkit
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
#include <xci/graphics/Window.h>
#include <xci/graphics/Shape.h>
#include <xci/util/file.h>
#include <xci/util/FpsCounter.h>
#include <cstdlib>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

using namespace xci::text;
using namespace xci::graphics;
using namespace xci::util;

int main()
{
    xci::util::chdir_to_share();

    Window& window = Window::default_window();
    window.create({800, 600}, "XCI fps counter demo");

    FontFace face;
    if (!face.load_from_file("fonts/ShareTechMono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;
    Font font;
    font.add_face(face);

    // normally, the border scales with viewport size
    Shape rts(Color(0, 0, 40, 128), Color(180, 180, 0));
    rts.set_antialiasing(2);
    rts.add_ellipse({-1, -0.6f, 2, 1.2f}, 0.05);
    rts.add_ellipse({-0.6f, -0.8f, 1.2f, 1.6f}, 0.02);

    // using View::screen_ratio, we can set constant border width, in screen pixels
    Shape rts_px(Color(40, 40, 0, 128), Color(255, 255, 0));
    rts_px.set_antialiasing(2);

    FpsCounter fps;
    double t_prev = 0;
    Text fps_text("0 fps", font);

    window.set_draw_callback([&](View& view) {
        rts.draw(view, {0, 0});

        auto pxr = view.screen_ratio().x;
        rts_px.add_ellipse({0.0f, 0.0f, 0.5f, 0.5f}, 1 * pxr);
        rts_px.add_ellipse({0.1f, 0.1f, 0.5f, 0.5f}, 2 * pxr);
        rts_px.add_ellipse({0.2f, 0.2f, 0.5f, 0.5f}, 3 * pxr);
        rts_px.add_ellipse({0.3f, 0.3f, 0.5f, 0.5f}, 4 * pxr);
        rts_px.add_ellipse({0.4f, 0.4f, 0.5f, 0.5f}, 5 * pxr);
        rts_px.draw(view, {-0.45f, -0.45f});
        rts_px.clear();

        double t_now = glfwGetTime();
        fps.tick(float(t_now - t_prev));
        t_prev = t_now;
        fps_text.set_string(std::to_string(fps.fps()) +
                            " fps{br}[v] vsync{br}[c] nowait");
        fps_text.draw(view, {-1.2f, -0.9f});
    });

    window.set_key_callback([&](View& view, KeyEvent ev){
        switch (ev.key) {
            case Key::V:
                window.set_refresh_mode(RefreshMode::PeriodicVsync);
                break;
            case Key::C:
                window.set_refresh_mode(RefreshMode::PeriodicNoWait);
                break;
            default:
                break;
        }
    });

    window.set_refresh_mode(RefreshMode::PeriodicNoWait);
    window.display();
    return EXIT_SUCCESS;
}
