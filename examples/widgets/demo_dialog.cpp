// demo_dialog.cpp created on 2023-12-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "graphics/common.h"

#include <xci/widgets/Dialog.h>
#include <xci/widgets/Label.h>
#include <xci/vfs/Vfs.h>
#include <xci/config.h>

#include <fmt/core.h>
#include <cstdlib>

using namespace xci::widgets;

int main(int, const char* argv[])
{
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};
    setup_window(window, "XCI dialog demo", argv);

    Theme theme(renderer);
    if (!theme.load_default())
        return EXIT_FAILURE;

    Composite root {theme};

    Dialog dialog {theme};
    dialog.set_position({-50_vp, -25_vp});
    dialog.set_width(100_vp);
    dialog.set_font_size(4_vp);
    dialog.set_color(Color::Grey());
    dialog.set_markup_string("The Dialog component allows creating interactive dialogs, "
                             "menus or general text with <s:selectable>selectable</s:selectable> spans.<p>"
                             "For example, this is a simple menu:<p>"
                             "<tab><s:a>a) Key A</s:a><p>"
                             "<tab><s:b>b) Key B</s:b><p>"
                             "The component itself doesn't draw any <s:bg>background</s:bg> or <s:frame>frame</s:frame>"
                             " - this needs to be provided externally, when needed.");
    dialog.create_items_from_spans();
    dialog.get_item("a")->key = Key::A;
    dialog.get_item("b")->key = Key::B;
    root.add_child(dialog);

    Label output_text(theme);
    output_text.set_position({-50_vp, -40_vp});
    output_text.set_font_size(4_vp);
    output_text.set_color(Color(180, 100, 140));
    dialog.on_activation([&output_text](View& view, Dialog::Item& item) {
        output_text.set_string(fmt::format("Activated: {}", item.span_name));
        output_text.resize(view);
    });
    root.add_child(output_text);

    window.set_refresh_mode(RefreshMode::Periodic);

    window.set_key_callback([&root, &window](View& view, const KeyEvent& ev) {
        if (ev.action != Action::Press)
            return;
        switch (ev.key) {
            case Key::Escape:
                window.close();
                break;
            case Key::F1:
                view.toggle_debug_flag(View::Debug::SpanBBox);
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
