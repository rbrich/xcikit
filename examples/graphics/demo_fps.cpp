// demo_fps.cpp created on 2018-04-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "common.h"

#include <xci/widgets/FpsDisplay.h>
#include <xci/text/Text.h>
#include <xci/graphics/Shape.h>
#include <xci/core/Vfs.h>
#include <xci/config.h>

#include <cstdlib>

using namespace xci::widgets;
using namespace xci::text;
using fmt::format;

int main(int argc, const char* argv[])
{
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};
    setup_window(window, "XCI fps counter demo", argv);

    Theme theme(renderer);
    if (!theme.load_default())
        return EXIT_FAILURE;
    auto& font = theme.font();

    Shape rts(renderer, Color(0, 0, 40, 128), Color(180, 180, 0));
    rts.set_antialiasing(2);

    Shape rts_px(renderer, Color(40, 40, 0, 128), Color(255, 255, 0));
    rts_px.set_antialiasing(2);

    FpsDisplay fps_display(theme);
    fps_display.set_position({-1.2_vp, -0.7_vp});
    Text help_text(font, "[p] periodic\t[i] immediate\n"
                         "[d] on demand\t[f] fifo\n"
                         "[e] on event\t[m] mailbox\n");
    Text mouse_pos(font, "Mouse: ");
    mouse_pos.set_color(Color(255, 150, 50));
    std::string mouse_pos_str;

    window.set_size_callback([&](View& view) {
        // Viewport units - the border scales with viewport size
        rts.clear();
        rts.add_ellipse(view.vp_to_fb({-1.f, -0.6f, 2.f, 1.2f}), view.vp_to_fb(0.05f));
        rts.add_ellipse(view.vp_to_fb({-0.6f, -0.8f, 1.2f, 1.6f}), view.vp_to_fb(0.02f));
        rts.update();

        // Constant border width, in screen pixels
        rts_px.clear();
        rts_px.add_ellipse(view.vp_to_fb({0.0f, 0.0f, 0.5f, 0.5f}), view.px_to_fb(1_px));
        rts_px.add_ellipse(view.vp_to_fb({0.1f, 0.1f, 0.5f, 0.5f}), view.px_to_fb(2_px));
        rts_px.add_ellipse(view.vp_to_fb({0.2f, 0.2f, 0.5f, 0.5f}), view.px_to_fb(3_px));
        rts_px.add_ellipse(view.vp_to_fb({0.3f, 0.3f, 0.5f, 0.5f}), view.px_to_fb(4_px));
        rts_px.add_ellipse(view.vp_to_fb({0.4f, 0.4f, 0.5f, 0.5f}), view.px_to_fb(5_px));
        rts_px.update();

        fps_display.resize(view);
        help_text.resize(view);
        mouse_pos.resize(view);
    });

    window.set_update_callback([&](View& view, std::chrono::nanoseconds elapsed){
        fps_display.update(view, State{ elapsed });
        if (!mouse_pos_str.empty()) {
            mouse_pos.set_fixed_string("Mouse: " + mouse_pos_str);
            mouse_pos.update(view);
            view.refresh();
            mouse_pos_str.clear();
        }
    });

    window.set_draw_callback([&](View& view) {
        rts.draw(view, {0_vp, 0_vp});
        rts_px.draw(view, {-0.45_vp, -0.45_vp});

        help_text.draw(view, {-1.2_vp, -0.9_vp});
        fps_display.draw(view);
        mouse_pos.draw(view, {-1.2_vp, 0.9_vp});
    });

    window.set_key_callback([&](View& view, KeyEvent ev){
        if (ev.action != Action::Press)
            return;
        switch (ev.key) {
            case Key::Escape:
                window.close();
                break;
            case Key::F11:
                window.toggle_fullscreen();
                break;
            case Key::P:
                window.set_refresh_mode(RefreshMode::Periodic);
                break;
            case Key::D:
                window.set_refresh_mode(RefreshMode::OnDemand);
                break;
            case Key::E:
                window.set_refresh_mode(RefreshMode::OnEvent);
                break;
            case Key::I:
                renderer.set_present_mode(PresentMode::Immediate);
                break;
            case Key::F:
                renderer.set_present_mode(PresentMode::Fifo);
                break;
            case Key::M:
                renderer.set_present_mode(PresentMode::Mailbox);
                break;
            default:
                break;
        }
    });

    window.set_mouse_position_callback([&](View& view, const MousePosEvent& ev) {
        mouse_pos_str = format("({}, {})", ev.pos.x, ev.pos.y);
    });

    window.set_refresh_mode(RefreshMode::Periodic);
    window.display();
    return EXIT_SUCCESS;
}
