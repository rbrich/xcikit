// demo_ui.cpp created on 2018-04-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "MousePosInfo.h"
#include "graphics/common.h"

#include <xci/widgets/Label.h>
#include <xci/widgets/Button.h>
#include <xci/widgets/Checkbox.h>
#include <xci/widgets/FpsDisplay.h>
#include <xci/widgets/TextInput.h>
#include <xci/widgets/Spinner.h>
#include <xci/widgets/ColorPicker.h>
#include <xci/vfs/Vfs.h>
#include <xci/config.h>
#include <random>
#include <deque>
#include <cstdlib>

// this brings in all required namespaces
using namespace xci::demo;

int main(int argc, const char* argv[])
{
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};
    setup_window(window, "XCI UI demo", argv);

    Theme theme(renderer);
    if (!theme.load_default())
        return EXIT_FAILURE;

    Composite root {theme};

    // Random color generator
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_int_distribution<int> runi(0, 255);
    auto random_color = [&](){ return Color(runi(re), runi(re), runi(re)); };

    // TextInput
    Label l_text_input(theme, "TextInput");
    l_text_input.set_position({-50_vp, -31_vp});
    l_text_input.set_color(Color::Cyan());
    root.add_child(l_text_input);

    std::deque<TextInput> text_inputs;
    for (auto i : {0,1,2,3,4}) {
        text_inputs.emplace_back(theme, "input");
        text_inputs.back().set_position({-50_vp, -25_vp + i * 6_vp});
        root.add_child(text_inputs.back());
    }

    // Button
    Label l_button(theme, "Button");
    l_button.set_position({-10_vp, -31_vp});
    l_button.set_color(Color::Cyan());
    root.add_child(l_button);

    std::deque<Button> buttons;
    for (auto i : {0,1,2,3,4}) {
        auto& btn = buttons.emplace_back(theme, std::to_string(i+1) + ". click me!");
        btn.set_position({-10_vp, -25_vp + i * 6_vp});
        buttons.back().on_click([&btn, &random_color](View& view) {
            view.finish_draw();
            btn.set_text_color(random_color());
            btn.resize(view);
        });
        root.add_child(btn);
    }

    // Checkbox
    Label l_checkbox(theme, "Checkbox");
    l_checkbox.set_position({25_vp, -31_vp});
    l_checkbox.set_color(Color::Cyan());
    root.add_child(l_checkbox);

    std::deque<Checkbox> checkboxes;
    for (auto i : {0,1,2,3,4}) {
        checkboxes.emplace_back(theme);
        checkboxes.back().set_text("Checkbox " + std::to_string(i+1));
        checkboxes.back().set_position({25_vp, -25_vp + i * 3_vp});
        root.add_child(checkboxes.back());
    }

    // Spinner
    Label l_spinner(theme, "Spinner");
    l_spinner.set_position({-50_vp, 8_vp});
    l_spinner.set_color(Color::Cyan());
    root.add_child(l_spinner);

    Spinner spinner1(theme, 0.5f);
    spinner1.set_position({-50_vp, 14_vp});
    root.add_child(spinner1);

    Spinner spinner2(theme, 0x80);
    spinner2.set_position({-40_vp, 14_vp});
    spinner2.set_format_cb([](float v){ return fmt::format("{:02X}", int(v)); });
    spinner2.set_step(1, 16);
    spinner2.set_bounds(0, 255);
    root.add_child(spinner2);

    // ColorPicker
    Label l_color_picker(theme, "ColorPicker");
    l_color_picker.set_position({-50_vp, 22_vp});
    l_color_picker.set_color(Color::Cyan());
    root.add_child(l_color_picker);

    ColorPicker color_picker(theme, Color::Magenta());
    color_picker.set_position({-50_vp, 28_vp});
    root.add_child(color_picker);

    MousePosInfo mouse_pos_info {theme};
    mouse_pos_info.set_position({-60_vp, 45_vp});
    root.add_child(mouse_pos_info);

    FpsDisplay fps_display {theme};
    fps_display.set_position({-60_vp, -40_vp});
    root.add_child(fps_display);

    window.set_key_callback([&](View& view, KeyEvent ev) {
        if (ev.action != Action::Press)
            return;
        switch (ev.key) {
            case Key::Escape:
                window.close();
                break;
            case Key::F11:
            case Key::F:
                window.toggle_fullscreen();
                break;
            case Key::R:
                fps_display.toggle_hidden();
                break;
            default:
                break;
        }
    });

    Bind bind(window, root);
    window.set_refresh_mode(RefreshMode::OnDemand);
    window.set_clear_color(Color(0, 0x19, 0x1C));
    window.display();
    return EXIT_SUCCESS;
}
