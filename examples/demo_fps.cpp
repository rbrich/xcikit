// demo_fps.cpp created on 2018-04-14, part of XCI toolkit
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

#include <xci/widgets/FpsDisplay.h>
#include <xci/text/Text.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/Shape.h>
#include <xci/core/Vfs.h>
#include <xci/core/format.h>
#include <xci/config.h>
#include <cstdlib>

using namespace xci::widgets;
using namespace xci::text;
using namespace xci::graphics;
using namespace xci::core;

int main()
{
    Vfs vfs;
    vfs.mount(XCI_SHARE_DIR);

    Window& window = Window::default_instance();
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

    FpsDisplay fps_display;
    fps_display.set_position({-1.2f, -0.7f});
    Text help_text(font, "[p] periodic\t[n] nowait\n"
                         "[d] on demand\t[v] vsync\n"
                         "[e] on event\t[h] halfrate\n");
    Text mouse_pos(font, "Mouse: ");
    mouse_pos.set_color(Color(255, 150, 50));

    window.set_update_callback([&](View& view, std::chrono::nanoseconds elapsed){
        fps_display.update(view, elapsed);
    });

    window.set_size_callback([&](View& view) {
        rts_px.clear();
        rts_px.add_ellipse({0.0f, 0.0f, 0.5f, 0.5f}, view.size_to_viewport(1_sc));
        rts_px.add_ellipse({0.1f, 0.1f, 0.5f, 0.5f}, view.size_to_viewport(2_sc));
        rts_px.add_ellipse({0.2f, 0.2f, 0.5f, 0.5f}, view.size_to_viewport(3_sc));
        rts_px.add_ellipse({0.3f, 0.3f, 0.5f, 0.5f}, view.size_to_viewport(4_sc));
        rts_px.add_ellipse({0.4f, 0.4f, 0.5f, 0.5f}, view.size_to_viewport(5_sc));
        fps_display.resize(view);
        help_text.resize(view);
        mouse_pos.resize(view);
    });

    window.set_draw_callback([&](View& view) {
        rts.draw(view, {0, 0});
        rts_px.draw(view, {-0.45f, -0.45f});

        help_text.draw(view, {-1.2f, -0.9f});
        fps_display.draw(view, {});
        mouse_pos.draw(view, {-1.2f, 0.9f});
    });

    window.set_key_callback([&](View& view, KeyEvent ev){
        if (ev.action != Action::Press)
            return;
        switch (ev.key) {
            case Key::P:
                window.set_refresh_mode(RefreshMode::Periodic);
                break;
            case Key::D:
                window.set_refresh_mode(RefreshMode::OnDemand);
                break;
            case Key::E:
                window.set_refresh_mode(RefreshMode::OnEvent);
                break;
            case Key::N:
                window.set_refresh_interval(0);
                break;
            case Key::V:
                window.set_refresh_interval(1);
                break;
            case Key::H:
                window.set_refresh_interval(2);
                break;
            default:
                break;
        }
    });

    window.set_mouse_position_callback([&](View& view, const MousePosEvent& ev) {
        mouse_pos.set_string("Mouse: " +
                             format("({}, {})", ev.pos.x, ev.pos.y));
        view.refresh();
    });

    window.set_refresh_mode(RefreshMode::Periodic);
    window.display();
    return EXIT_SUCCESS;
}
