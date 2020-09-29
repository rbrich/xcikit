// demo_widget.cpp created on 2018-03-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/widgets/Theme.h>
#include <xci/widgets/Button.h>
#include <xci/widgets/Icon.h>
#include <xci/text/Text.h>
#include <xci/graphics/Window.h>
#include <xci/core/Vfs.h>
#include <xci/core/log.h>
#include <xci/config.h>

#include <fmt/ostream.h>
#include <cstdlib>

using namespace xci::widgets;
using namespace xci::text;
using namespace xci::graphics;
using namespace xci::core;
using namespace xci::core::log;

int main()
{
    Vfs vfs;
    vfs.mount(XCI_SHARE);

    Renderer renderer {vfs};
    Window window {renderer};
    window.create({800, 600}, "XCI widgets demo");

    Theme theme(renderer);
    if (!theme.load_default())
        return EXIT_FAILURE;

    Button button_default(theme, "Default button");
    button_default.set_position({0, -0.2f});

    Button button_styled(theme, "Styled button");
    button_styled.set_font_size(0.07);
    button_styled.set_padding(0.05);
    button_styled.set_decoration_color(Color(10, 20, 100), Color(20, 50, 150));
    button_styled.set_text_color(Color(255, 255, 50));

    Icon checkbox(theme);
    checkbox.set_position({0, 0.4f});
    checkbox.set_icon(IconId::CheckBoxChecked);
    checkbox.set_text("Checkbox");
    checkbox.set_font_size(0.08);
    checkbox.set_color(Color(150, 200, 200));
    bool checkbox_state = true;
    bool checkbox_active = false;
    bool refresh_checkbox = true;

    window.set_size_callback([&](View& view) {
        //view.set_debug_flag(View::Debug::WordBasePoint);
        //view.set_debug_flag(View::Debug::PageBBox);
        button_default.resize(view);
        button_styled.set_outline_thickness(view.size_to_viewport(1_sc));
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
            debug("checkbox mouse {}", ev.pos - view.offset());
            debug("checkbox bbox {}", checkbox.aabb());
            if (checkbox.contains(ev.pos - view.offset())) {
                checkbox_state = !checkbox_state;
                debug("checkbox state {}", checkbox_state);
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

    window.display();
    return EXIT_SUCCESS;
}
