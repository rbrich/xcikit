// demo_coords.cpp created on 2018-03-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "common.h"

#include <xci/text/Font.h>
#include <xci/text/Text.h>
#include <xci/graphics/Sprites.h>
#include <xci/graphics/shape/Rectangle.h>
#include <xci/graphics/Color.h>
#include <xci/vfs/Vfs.h>
#include <xci/config.h>

#include <cstdlib>

using namespace xci::text;
using namespace xci::graphics::unit_literals;
using fmt::format;

int main(int argc, const char* argv[])
{
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};

    setup_window(window, "XCI coords demo", argv);

    Font font {renderer};
    if (!font.add_face(vfs, "fonts/ShareTechMono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;

    Text coords_center(font, "(0, 0)");
    Text coords_tl(font, "(-, -)");
    Text coords_br(font, "(-, -)");
    Text coords_tr(font, "(-, -)");
    Text coords_bl(font, "(-, -)");
    Text size_viewport(font, "Viewport size:         ");
    size_viewport.set_color(Color(130, 120, 255));
    Text size_screen(font, "Screen size:       ");
    size_screen.set_color(Color(110, 130, 255));
    Text size_frame(font, "Framebuffer size:  ");
    size_frame.set_color(Color(90, 140, 255));
    Text size_font(font, "Font size:         ");
    size_font.set_color(Color(70, 150, 255));
    Text mouse_pos(font, "Mouse position:    ");
    mouse_pos.set_color(Color(255, 150, 50));
    std::string mouse_pos_str;

    Text help_text(font, "Units:     \tOrigin:     \tScale:\n"
                         "[s] scaling\t[c] center  \t[+] bigger\n"
                         "[f] fixed  \t[t] top-left\t[-] smaller\n");
    help_text.set_color(Color(200, 100, 50));

    Rectangle unit_square(renderer);

    bool scaling = true;
    ViewOrigin view_origin = ViewOrigin::Center;
    VariUnits font_size = 2.5_vp;

    window.set_size_callback([&](View& view) {
        auto vs = view.viewport_size();
        auto ps = view.screen_size();
        auto fs = view.framebuffer_size();
        coords_center.set_font_size(font_size);
        coords_tl.set_font_size(font_size);
        coords_br.set_font_size(font_size);
        coords_tr.set_font_size(font_size);
        coords_bl.set_font_size(font_size);
        size_viewport.set_font_size(font_size);
        size_screen.set_font_size(font_size);
        size_frame.set_font_size(font_size);
        size_font.set_font_size(font_size);
        mouse_pos.set_font_size(font_size);
        help_text.set_font_size(font_size);

        if (scaling) {
            auto c = view.viewport_center();
            coords_tl.set_fixed_string(format("({}, {})", c.x - 0.5f * vs.x, c.y - 0.5f * vs.y));
            coords_br.set_fixed_string(format("({}, {})", c.x + 0.5f * vs.x, c.y + 0.5f * vs.y));
            coords_tr.set_fixed_string(format("({}, {})", c.x + 0.5f * vs.x, c.y - 0.5f * vs.y));
            coords_bl.set_fixed_string(format("({}, {})", c.x - 0.5f * vs.x, c.y + 0.5f * vs.y));
            mouse_pos.set_tab_stops({20_vp});
            help_text.set_tab_stops({20_vp, 20_vp});
        } else {
            auto c = view.screen_center();
            coords_tl.set_fixed_string(format("({}, {})", c.x - 0.5f * ps.x, c.y - 0.5f * ps.y));
            coords_br.set_fixed_string(format("({}, {})", c.x + 0.5f * ps.x, c.y + 0.5f * ps.y));
            coords_tr.set_fixed_string(format("({}, {})", c.x + 0.5f * ps.x, c.y - 0.5f * ps.y));
            coords_bl.set_fixed_string(format("({}, {})", c.x - 0.5f * ps.x, c.y + 0.5f * ps.y));
            mouse_pos.set_tab_stops({120_px});
            help_text.set_tab_stops({120_px, 120_px});
        }
        coords_center.resize(view);
        coords_tl.resize(view);
        coords_br.resize(view);
        coords_tr.resize(view);
        coords_bl.resize(view);
        help_text.resize(view);

        float scale = view.viewport_scale();
        size_viewport.set_fixed_string(
                "Viewport size:     " +
                format("{} x {}", vs.x, vs.y) +
                "  (" + format("{} x {}", scale, scale) + ")");
        size_viewport.resize(view);

        size_screen.set_fixed_string(
                "Screen size:       " +
                format("{} x {}", ps.x, ps.y) +
                "  (" + format("{} x {}", ps.x * scale / vs.x.value, ps.y * scale / vs.y.value) + ")");
        size_screen.resize(view);

        size_frame.set_fixed_string(
                "Framebuffer size:  " +
                format("{} x {}", fs.x, fs.y) +
                "  (" + format("{} x {}", fs.x * scale / vs.x.value, fs.y * scale / vs.y.value) + ")");
        size_frame.resize(view);

        size_font.set_fixed_string(format("Font size:         {}", font.size()));
        size_font.resize(view);

        mouse_pos.resize(view);

        unit_square.clear();
        if (view_origin == ViewOrigin::Center)
            unit_square.add_rectangle(view.vp_to_fb({-50, -50, 100, 100}), view.px_to_fb(1_px));
        else
            unit_square.add_rectangle(view.vp_to_fb({0, 0, 100, 100}), view.px_to_fb(1_px));
        unit_square.update(Color::Transparent(), Color::Grey());
    });

    window.set_draw_callback([&](View& view) {
        if (scaling)
            unit_square.draw(view, {0_vp, 0_vp});

        if (view_origin == ViewOrigin::Center)
            coords_center.draw(view, {0_vp, 0_vp});

        if (scaling) {
            auto vs = view.viewport_size();
            auto vc = view.viewport_center();
            coords_tl.draw(view, {vc.x - 0.45f * vs.x, vc.y - 0.45f * vs.y});
            coords_br.draw(view, {vc.x + 0.30f * vs.x, vc.y + 0.45f * vs.y});
            coords_tr.draw(view, {vc.x + 0.30f * vs.x, vc.y - 0.45f * vs.y});
            coords_bl.draw(view, {vc.x - 0.45f * vs.x, vc.y + 0.45f * vs.y});
            size_viewport.draw(view, {vc.x - 20_vp, vc.y - 25_vp});
            size_screen.draw(view, {vc.x - 20_vp, vc.y - 20_vp});
            size_frame.draw(view, {vc.x - 20_vp, vc.y - 15_vp});
            size_font.draw(view, {vc.x - 20_vp, vc.y - 10_vp});
            mouse_pos.draw(view, {vc.x - 20_vp, vc.y + 10_vp});
            help_text.draw(view, {vc.x - 20_vp, vc.y + 25_vp});
        } else {
            auto size = view.screen_size();
            auto sc = view.screen_center();
            auto tl = sc - 0.5 * size;
            auto br = sc + 0.5 * size;
            coords_tl.draw(view, {tl.x + 30_px, tl.y + 30_px});
            coords_br.draw(view, {br.x - 150_px, br.y - 30_px});
            coords_tr.draw(view, {br.x - 150_px, tl.y + 30_px});
            coords_bl.draw(view, {tl.x + 30_px, br.y - 30_px});
            size_viewport.draw(view, {sc.x - 120_px, sc.y - 150_px});
            size_screen.draw(view, {sc.x - 120_px, sc.y - 120_px});
            size_frame.draw(view, {sc.x - 120_px, sc.y - 90_px});
            size_font.draw(view, {sc.x - 120_px, sc.y - 60_px});
            mouse_pos.draw(view, {sc.x - 120_px, sc.y + 60_px});
            help_text.draw(view, {sc.x - 120_px, sc.y + 120_px});
        }
    });

    window.set_key_callback([&](View& view, KeyEvent ev){
        if (ev.action != Action::Press)
            return;
        bool refresh = false;
        switch (ev.key) {
            case Key::Escape:
                window.close();
                break;
            case Key::F11:
                window.toggle_fullscreen();
                break;
            case Key::S:
                scaling = true;
                font_size = 2.5_vp;
                refresh = true;
                break;
            case Key::F:
                scaling = false;
                font_size = 15_px;
                refresh = true;
                break;
            case Key::C:
                view_origin = ViewOrigin::Center;
                window.set_view_origin(view_origin);
                refresh = true;
                break;
            case Key::T:
                view_origin = ViewOrigin::TopLeft;
                window.set_view_origin(view_origin);
                refresh = true;
                break;
            case Key::Equal:
            case Key::KeypadAdd: {
                float scale = view.viewport_scale();
                if (scale < 200)
                    scale += 5;
                view.set_viewport_scale(scale);
                refresh = true;
                break;
            }
            case Key::Minus:
            case Key::KeypadSubtract: {
                float scale = view.viewport_scale();
                if (scale > 50)
                    scale -= 5;
                view.set_viewport_scale(scale);
                refresh = true;
                break;
            }
            default:
                break;
        }
        if (refresh) {
            window.size_callback()(view);
            view.refresh();
        }
    });

    window.set_update_callback([&](View& view, std::chrono::nanoseconds) {
        if (!mouse_pos_str.empty()) {
            mouse_pos.set_string("Mouse position:" + mouse_pos_str);
            mouse_pos.update(view);
            view.refresh();
            mouse_pos_str.clear();
        }
    });

    window.set_mouse_position_callback([&](View& view, const MousePosEvent& ev) {
        auto pos_px = view.fb_to_px(ev.pos);
        auto pos_vp = view.fb_to_vp(ev.pos);
        mouse_pos_str = format("\t({}, {}) vp\n\t({}, {}) px\n\t({}, {}) fb",
                               pos_vp.x, pos_vp.y, pos_px.x, pos_px.y, ev.pos.x, ev.pos.y);
    });

    window.display();
    return EXIT_SUCCESS;
}
