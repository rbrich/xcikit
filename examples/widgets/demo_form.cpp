// demo_form.cpp created on 2018-06-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "MousePosInfo.h"
#include "graphics/common.h"

#include <xci/widgets/Button.h>
#include <xci/widgets/FpsDisplay.h>
#include <xci/widgets/Form.h>
#include <xci/widgets/Label.h>
#include <xci/vfs/Vfs.h>
#include <xci/config.h>

#include <fmt/core.h>
#include <iostream>
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
    setup_window(window, "XCI form demo", argv);

    Theme theme(renderer);
    if (!theme.load_default())
        return EXIT_FAILURE;

    Composite root {theme};

    // Form #1
    Form form1 {theme};
    form1.set_position({-50_vp, -25_vp});
    root.add_child(form1);

    std::string input_text = "2018-06-23";
    form1.add_input("date", input_text);

    bool checkbox1 = false;
    form1.add_input("checkbox1", checkbox1);

    bool checkbox2 = true;
    form1.add_input("checkbox2", checkbox2);

    Button button(theme, "submit");
    form1.add_hint(Form::Hint::NextColumn);
    form1.add_child(button);

    // Form #1 output
    Label output_text(theme);
    output_text.set_position({10_vp, -25_vp});
    output_text.set_color(Color(180, 100, 140));
    button.on_click([&output_text, &input_text, &checkbox1, &checkbox2]
                     (View& view) {
        auto text = fmt::format("Submitted:\n\n"
                                "input_text = {}\n\n"
                                "checkbox1 = {}\n\n"
                                "checkbox2 = {}\n\n",
                                input_text, checkbox1, checkbox2);
        output_text.set_string(text);
        output_text.resize(view);
    });
    root.add_child(output_text);

    // Form #2
    Form form2(theme);
    form2.set_position({-50_vp, 10_vp});
    root.add_child(form2);

    std::string name = "Player1";
    form2.add_input("name", name);

    bool hardcore = false;
    form2.add_input("hardcore", hardcore);

    // Mouse pos
    MousePosInfo mouse_pos_info(theme);
    mouse_pos_info.set_position({-60_vp, 45_vp});
    root.add_child(mouse_pos_info);

    // FPS
    FpsDisplay fps_display(theme);
    fps_display.set_position({-60_vp, -40_vp});
    root.add_child(fps_display);

    window.set_refresh_mode(RefreshMode::OnDemand);
    //window.set_debug_flags(View::DebugFlags(View::Debug::LineBaseLine));

    window.set_key_callback([&root, &window](View& v, const KeyEvent& ev) {
        if (ev.action != Action::Press)
            return;
        switch (ev.key) {
            case Key::Escape:
                window.close();
                break;
            case Key::F11:
                window.toggle_fullscreen();
                break;
            case Key::D:
                root.dump(std::cout);
                std::cout << std::endl;
                break;
            default:
                break;
        }
    });

    Bind bind(window, root);
    window.display();
    return EXIT_SUCCESS;
}
