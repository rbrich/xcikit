// demo_ui.cpp created on 2018-04-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "MousePosInfo.h"
#include "graphics/common.h"

#include <xci/widgets/Button.h>
#include <xci/widgets/Checkbox.h>
#include <xci/widgets/FpsDisplay.h>
#include <xci/widgets/TextInput.h>
#include <xci/core/Vfs.h>
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

    std::deque<TextInput> text_inputs;
    for (auto i : {0,1,2,3,4}) {
        text_inputs.emplace_back(theme, "input");
        text_inputs.back().set_position({-1.0f, -0.5f + i * 0.12f});
        root.add(text_inputs.back());
    }

    std::deque<Button> buttons;
    for (auto i : {0,1,2,3,4}) {
        buttons.emplace_back(theme, std::to_string(i+1) + ". click me!");
        buttons.back().set_position({-0.2f, -0.5f + i * 0.12f});
        auto* btn_self = &buttons.back();
        buttons.back().on_click([btn_self, &random_color](View& view) {
            view.finish_draw();
            btn_self->set_text_color(random_color());
            btn_self->resize(view);
        });
        root.add(buttons.back());
    }

    std::deque<Checkbox> checkboxes;
    for (auto i : {0,1,2,3,4}) {
        checkboxes.emplace_back(theme);
        checkboxes.back().set_text("Checkbox " + std::to_string(i+1));
        checkboxes.back().set_position({0.5f, -0.5f + i * 0.06f});
        root.add(checkboxes.back());
    }

    MousePosInfo mouse_pos_info {theme};
    mouse_pos_info.set_position({-1.2f, 0.9f});
    root.add(mouse_pos_info);

    FpsDisplay fps_display {theme};
    fps_display.set_position({-1.2f, -0.8f});
    root.add(fps_display);

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

    Bind bind(window, root);
    window.set_refresh_mode(RefreshMode::OnDemand);
    window.display();
    return EXIT_SUCCESS;
}
