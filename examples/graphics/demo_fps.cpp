// demo_fps.cpp created on 2018-04-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "common.h"

#include <xci/widgets/FpsDisplay.h>
#include <xci/text/Text.h>
#include <xci/graphics/shape/Ellipse.h>
#include <xci/vfs/Vfs.h>
#include <xci/config.h>

#include <cstdlib>

using namespace xci::widgets;
using namespace xci::text;
using namespace xci::vfs;
using fmt::format;

int main(int argc, char* argv[])
{
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};
    setup_window(window, "XCI fps counter demo", argv);

    Theme theme(window);
    if (!theme.load_default())
        return EXIT_FAILURE;
    auto& font = theme.base_font();

    Ellipse rts(renderer);
    Ellipse rts_px(renderer);

    FpsDisplay fps_display(theme);
    fps_display.set_position({-60_vp, -35_vp});
    Text help_text(font, "[p] periodic\t[i] immediate\n"
                         "[d] on demand\t[f] fifo\t[r] relaxed\n"
                         "[e] on event\t[m] mailbox\n");
    Text mouse_pos(font, "Mouse: ");
    mouse_pos.set_color(Color(255, 150, 50));
    std::string mouse_pos_str;

    window.set_size_callback([&](View& view) {
        // Viewport units - the border scales with viewport size
        EllipseBuilder(view, rts)
            .set_antialiasing(2)
            .set_fill_color(Color(0, 0, 40, 128))
            .set_outline_color(Color(180, 180, 0))
            .add_ellipse({-50_vp, -30_vp, 100_vp, 60_vp}, 2.5_vp)
            .add_ellipse({-30_vp, -40_vp, 60_vp, 80_vp}, 1_vp);

        // Constant border width, in screen pixels
        EllipseBuilder(view, rts_px)
            .set_antialiasing(2)
            .set_fill_color(Color(40, 40, 0, 128))
            .set_outline_color(Color(255, 255, 0))
            .add_ellipse({ 0_vp,  0_vp, 25_vp, 25_vp}, 1_px)
            .add_ellipse({ 5_vp,  5_vp, 25_vp, 25_vp}, 2_px)
            .add_ellipse({10_vp, 10_vp, 25_vp, 25_vp}, 3_px)
            .add_ellipse({15_vp, 15_vp, 25_vp, 25_vp}, 4_px)
            .add_ellipse({20_vp, 20_vp, 25_vp, 25_vp}, 5_px);

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
        rts_px.draw(view, {-22.5_vp, -22.5_vp});

        help_text.draw(view, {-60_vp, -45_vp});
        fps_display.draw(view);
        mouse_pos.draw(view, {-60_vp, 45_vp});
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
            case Key::R:
                renderer.set_present_mode(PresentMode::FifoRelaxed);
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
