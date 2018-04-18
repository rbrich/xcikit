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

#include <xci/widgets/FpsDisplay.h>
#include <xci/text/Text.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/Shape.h>
#include <xci/util/FpsCounter.h>
#include <xci/util/file.h>
#include <xci/util/format.h>
#include <cstdlib>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

using namespace xci::widgets;
using namespace xci::text;
using namespace xci::graphics;
using namespace xci::util;

int main()
{
    xci::util::chdir_to_share();

    Window& window = Window::default_window();
    window.create({800, 600}, "XCI fps counter demo");

    if (!Theme::load_default_theme())
        return EXIT_FAILURE;
    auto& font = Theme::default_theme().font();

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
    FpsDisplay fps_display(fps);
    Text help_text("[v] vsync{tab}[d] on demand{br}[n] nowait{tab}[e] on event", font);
    Text mouse_pos("Mouse: ", font);
    mouse_pos.set_color(Color(255, 150, 50));

    window.set_size_callback([&](View& view) {
        auto pxr = view.screen_ratio().x;
        rts_px.clear();
        rts_px.add_ellipse({0.0f, 0.0f, 0.5f, 0.5f}, 1 * pxr);
        rts_px.add_ellipse({0.1f, 0.1f, 0.5f, 0.5f}, 2 * pxr);
        rts_px.add_ellipse({0.2f, 0.2f, 0.5f, 0.5f}, 3 * pxr);
        rts_px.add_ellipse({0.3f, 0.3f, 0.5f, 0.5f}, 4 * pxr);
        rts_px.add_ellipse({0.4f, 0.4f, 0.5f, 0.5f}, 5 * pxr);
        fps_display.resize(view);
    });

    window.set_draw_callback([&](View& view) {
        rts.draw(view, {0, 0});
        rts_px.draw(view, {-0.45f, -0.45f});

        double t_now = glfwGetTime();
        fps.tick(float(t_now - t_prev));
        t_prev = t_now;

        help_text.draw(view, {-1.2f, -0.9f});
        fps_display.draw(view, {-1.2f, -0.8f});
        mouse_pos.draw(view, {-1.2f, 0.9f});
    });

    window.set_key_callback([&](View& view, KeyEvent ev){
        switch (ev.key) {
            case Key::V:
                window.set_refresh_mode(RefreshMode::PeriodicVsync);
                break;
            case Key::N:
                window.set_refresh_mode(RefreshMode::PeriodicNoWait);
                break;
            case Key::D:
                window.set_refresh_mode(RefreshMode::OnDemand);
                break;
            case Key::E:
                window.set_refresh_mode(RefreshMode::OnEvent);
                break;
            default:
                break;
        }
    });

    window.set_mouse_position_callback([&](View& view, const Vec2f& pos) {
        mouse_pos.set_fixed_string("Mouse: " +
                                   format("({}, {})", pos.x, pos.y));
        view.refresh();
    });

    window.set_refresh_mode(RefreshMode::PeriodicVsync);
    window.display();
    return EXIT_SUCCESS;
}
