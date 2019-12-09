// demo_coords.cpp created on 2018-03-18, part of XCI toolkit
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


#include <xci/text/Font.h>
#include <xci/text/Text.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/Sprites.h>
#include <xci/graphics/Shape.h>
#include <xci/graphics/Color.h>
#include <xci/core/format.h>
#include <xci/core/Vfs.h>
#include <xci/config.h>
#include <cstdlib>

using namespace xci::text;
using namespace xci::graphics;
using namespace xci::graphics::unit_literals;
using namespace xci::core;

int main()
{
    Vfs vfs;
    vfs.mount(XCI_SHARE_DIR);

    Window& window = Window::default_instance();
    window.create({800, 600}, "XCI coords demo");

    Font font;
    if (!font.add_face(vfs, "fonts/ShareTechMono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;

    Text coords_center(font, "(0, 0)");
    Text coords_tl(font, "(-, -)");
    Text coords_br(font, "(-, -)");
    Text coords_tr(font, "(-, -)");
    Text coords_bl(font, "(-, -)");
    Text size_scal(font, "Viewport size:         ");
    size_scal.set_color(Color(130, 120, 255));
    Text size_screen(font, "Screen size:       ");
    size_screen.set_color(Color(110, 130, 255));
    Text size_frame(font, "Framebuffer size:  ");
    size_frame.set_color(Color(90, 140, 255));
    Text size_font(font, "Font size:         ");
    size_font.set_color(Color(70, 150, 255));
    Text mouse_pos(font, "Mouse position:    ");
    mouse_pos.set_color(Color(255, 150, 50));

    Text help_text(font, "Units:     \tOrigin:\n"
                         "[s] scaling\t[c] center\n"
                         "[f] fixed  \t[t] top-left\n");

    Shape unit_square(Color::Transparent(), Color(0.7, 0.7, 0.7));

    ViewScale view_scale = ViewScale::ScalingWithAspectCorrection;
    ViewOrigin view_origin = ViewOrigin::Center;
    ViewportUnits font_size = 0.05;

    window.set_size_callback([&](View& view) {
        auto vs = view.viewport_size();
        auto ps = view.screen_size();
        auto fs = view.framebuffer_size();
        auto vc = view.viewport_center();
        coords_center.set_font_size(font_size);
        coords_tl.set_font_size(font_size);
        coords_br.set_font_size(font_size);
        coords_tr.set_font_size(font_size);
        coords_bl.set_font_size(font_size);
        size_scal.set_font_size(font_size);
        size_screen.set_font_size(font_size);
        size_frame.set_font_size(font_size);
        size_font.set_font_size(font_size);
        mouse_pos.set_font_size(font_size);
        help_text.set_font_size(font_size);

        coords_tl.set_fixed_string(format("({}, {})", vc.x - 0.5f * vs.x, vc.y - 0.5f * vs.y));
        coords_br.set_fixed_string(format("({}, {})", vc.x + 0.5f * vs.x, vc.y + 0.5f * vs.y));
        coords_tr.set_fixed_string(format("({}, {})", vc.x + 0.5f * vs.x, vc.y - 0.5f * vs.y));
        coords_bl.set_fixed_string(format("({}, {})", vc.x - 0.5f * vs.x, vc.y + 0.5f * vs.y));
        coords_center.resize(view);
        coords_tl.resize(view);
        coords_br.resize(view);
        coords_tr.resize(view);
        coords_bl.resize(view);

        size_scal.set_fixed_string("Viewport size:     " +
                                   format("{} x {}", vs.x, vs.y) +
                                   "  (1 x 1)");
        size_scal.resize(view);

        size_screen.set_fixed_string("Screen size:       " +
                                     format("{} x {}", ps.x, ps.y) +
                                     "  (" + format("{} x {}", ps.x.value/vs.x.value, ps.y.value/vs.y.value) + ")");
        size_screen.resize(view);

        size_frame.set_fixed_string("Framebuffer size:  " +
                                    format("{} x {}", fs.x, fs.y) +
                                    "  (" + format("{} x {}", fs.x.value/vs.x.value, fs.y.value/vs.y.value) + ")");
        size_frame.resize(view);

        unit_square.clear();
        if (view_origin == ViewOrigin::Center)
            unit_square.add_rectangle({-1, -1, 2, 2}, view.size_to_viewport(1_sc));
        else
            unit_square.add_rectangle({0, 0, 2, 2}, view.size_to_viewport(1_sc));
    });

    window.set_draw_callback([&](View& view) {
        size_font.set_fixed_string(format("Font size:         {}", font.size()));

        if (view_scale == ViewScale::ScalingWithAspectCorrection)
            unit_square.draw(view, {0,0});

        auto vs = view.viewport_size();
        auto vc = view.viewport_center();

        if (view_origin == ViewOrigin::Center)
            coords_center.draw(view, {0.0f, 0.0f});

        if (view_scale == ViewScale::ScalingWithAspectCorrection) {
            coords_tl.draw(view, {vc.x - 0.45f * vs.x, vc.y - 0.45f * vs.y});
            coords_br.draw(view, {vc.x + 0.30f * vs.x, vc.y + 0.45f * vs.y});
            coords_tr.draw(view, {vc.x + 0.30f * vs.x, vc.y - 0.45f * vs.y});
            coords_bl.draw(view, {vc.x - 0.45f * vs.x, vc.y + 0.45f * vs.y});
            size_scal.draw(view, {vc.x - 0.4f, vc.y - 0.5f});
            size_screen.draw(view, {vc.x - 0.4f, vc.y - 0.4f});
            size_frame.draw(view, {vc.x - 0.4f, vc.y - 0.3f});
            size_font.draw(view, {vc.x - 0.4f, vc.y - 0.2f});
            mouse_pos.draw(view, {vc.x - 0.4f, vc.y + 0.2f});
            help_text.draw(view, {vc.x - 0.4f, vc.y + 0.3f});
        } else {
            auto tl = vc - vs / 2.0_vp;
            auto br = vc + vs / 2.0_vp;
            coords_tl.draw(view, {tl.x + 30, tl.y + 30});
            coords_br.draw(view, {br.x - 150, br.y - 30});
            coords_tr.draw(view, {br.x - 150, tl.y + 30});
            coords_bl.draw(view, {tl.x + 30, br.y - 30});
            size_scal.draw(view, {vc.x - 120, vc.y - 150});
            size_screen.draw(view, {vc.x - 120, vc.y - 120});
            size_frame.draw(view, {vc.x - 120, vc.y - 90});
            size_font.draw(view, {vc.x - 120, vc.y - 60});
            mouse_pos.draw(view, {vc.x - 120, vc.y + 60});
            help_text.draw(view, {vc.x - 120, vc.y + 90});
        }
    });

    window.set_key_callback([&](View& view, KeyEvent ev){
        if (ev.action != Action::Press)
            return;
        switch (ev.key) {
            case Key::S:
                view_scale = ViewScale::ScalingWithAspectCorrection;
                font_size = 0.05;
                window.set_view_mode(view_origin, view_scale);
                window.size_callback()(view);
                view.refresh();
                break;
            case Key::F:
                view_scale = ViewScale::FixedScreenPixels;
                font_size = 15.0;
                window.set_view_mode(view_origin, view_scale);
                window.size_callback()(view);
                view.refresh();
                break;
            case Key::C:
                view_origin = ViewOrigin::Center;
                window.set_view_mode(view_origin, view_scale);
                window.size_callback()(view);
                view.refresh();
                break;
            case Key::T:
                view_origin = ViewOrigin::TopLeft;
                window.set_view_mode(view_origin, view_scale);
                window.size_callback()(view);
                view.refresh();
                break;
            default:
                break;
        }
    });

    window.set_mouse_position_callback([&](View& view, const MousePosEvent& ev) {
        mouse_pos.set_fixed_string("Mouse position:    " +
                                   format("({}, {})", ev.pos.x, ev.pos.y));
        mouse_pos.resize(view);
        view.refresh();
    });

    window.display();
    return EXIT_SUCCESS;
}
