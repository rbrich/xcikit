// demo_widget.cpp created on 2018-03-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "graphics/common.h"

#include <xci/widgets/Theme.h>
#include <xci/widgets/Button.h>
#include <xci/widgets/Icon.h>
#include <xci/vfs/Vfs.h>
#include <xci/core/log.h>
#include <xci/config.h>

#include <cstdlib>

using namespace xci::widgets;
using namespace xci::text;

int main(int argc, const char* argv[])
{
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};
    setup_window(window, "XCI widgets demo", argv);

    Theme theme(renderer);
    if (!theme.load_default())
        return EXIT_FAILURE;

    Button button_default(theme, "Default button");
    button_default.set_position({0_vp, -10_vp});

    Button button_styled(theme, "Styled button");
    button_styled.set_font_size(3.5_vp);
    button_styled.set_padding(2.5_vp);
    button_styled.set_decoration_color(Color(10, 20, 100), Color(20, 50, 150));
    button_styled.set_text_color(Color(255, 255, 50));

    Icon checkbox(theme);
    checkbox.set_position({0_vp, 20_vp});
    checkbox.set_icon(IconId::CheckBoxChecked);
    checkbox.set_text("Checkbox");
    checkbox.set_font_size(4_vp);
    checkbox.set_color(Color(150, 200, 200));
    bool checkbox_state = true;
    bool checkbox_active = false;
    bool refresh_checkbox = true;

    window.set_size_callback([&](View& view) {
        //view.set_debug_flag(View::Debug::WordBasePoint);
        //view.set_debug_flag(View::Debug::PageBBox);
        button_default.resize(view);
        button_styled.set_outline_thickness(1_px);
        button_styled.resize(view);
        checkbox.resize(view);
    });

    window.set_update_callback([&](View& view, std::chrono::nanoseconds elapsed) {
        if (refresh_checkbox) {
            refresh_checkbox = false;
            checkbox.set_icon(checkbox_state
                    ? IconId::CheckBoxChecked
                    : IconId::CheckBoxUnchecked);
            checkbox.resize(view);
            checkbox.update(view, State{ .focused = checkbox_active });
            view.refresh();
        }
    });

    window.set_draw_callback([&](View& view) {
        button_default.draw(view);
        button_styled.draw(view);
        checkbox.draw(view);
    });

    window.set_mouse_button_callback([&](View& view, const MouseBtnEvent& ev) {
        if (ev.action == Action::Press && ev.button == MouseButton::Left) {
            log::debug("checkbox mouse {}", ev.pos - view.offset());
            log::debug("checkbox bbox {}", checkbox.aabb());
            if (checkbox.contains(ev.pos - view.offset())) {
                checkbox_state = !checkbox_state;
                log::debug("checkbox state {}", checkbox_state);
                refresh_checkbox = true;
            }
        }
    });

    window.set_mouse_position_callback([&](View& view, const MousePosEvent& ev) {
        bool mouse_in = checkbox.contains(ev.pos - view.offset());
        if (mouse_in != checkbox_active) {
            checkbox_active = mouse_in;
            refresh_checkbox = true;
        }
    });

    window.set_key_callback([&](View& view, KeyEvent ev) {
        if (ev.action != Action::Press)
            return;
        switch (ev.key) {
            case Key::Escape:
                window.close();
                break;
            case Key::F11:
                window.toggle_fullscreen();
                break;
            default:
                break;
        }
    });

    window.display();
    return EXIT_SUCCESS;
}
