// demo_outline.cpp created on 2021-11-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "graphics/common.h"

#include <xci/text/Font.h>
#include <xci/text/Text.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/Sprites.h>
#include <xci/core/Vfs.h>
#include <xci/config.h>
#include <cstdlib>

using namespace xci::text;
using namespace xci::graphics;
using namespace xci::core;

static const char * sample_text =
"Text without outline. <i>Text without outline</i>\n\n"
"<s:white_outline>Transparent text with white outline. <i>Transparent text with white outline.</i></s:white_outline>\n\n"
"<s:black_white>Black text with white outline. <i>Black text with white outline.</i></s:black_white>\n\n"
"<s:blue_white>Blue text with white outline. <i>Blue text with white outline.</i></s:blue_white>\n\n"
"<s:white_red>White text with red outline. <i>White text with red outline.</i></s:white_red>\n\n"
"";


int main(int argc, const char* argv[])
{
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};

    setup_window(window, "XCI layout demo", argv);

    Font font {renderer, 1024u};
    if (!font.add_face(vfs, "fonts/Lora/Lora[wght].ttf", 0))
        return EXIT_FAILURE;
    if (!font.add_face(vfs, "fonts/Lora/Lora-Italic[wght].ttf", 0))
        return EXIT_FAILURE;

    Font mono_font{renderer};
    if (!mono_font.add_face(vfs, "fonts/ShareTechMono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;

    ScreenPixels outline_radius = 1_px;

    Text text;
    text.set_markup_string(sample_text);
    text.set_width(66.5_vp);
    text.set_font(font);
    text.set_font_size(4.5_vp);
    text.set_outline_radius(outline_radius);

    auto apply_spans = [&layout = text.layout()](const View& view) {
        layout.get_span("black_white")->adjust_style([](Style& s) {
            s.set_color(Color::Black());
            s.set_outline_color(Color::White());
        });
        layout.get_span("blue_white")->adjust_style([](Style& s) {
            s.set_color(Color::Blue());
            s.set_outline_color(Color::White());
        });
        layout.get_span("white_outline")->adjust_style([](Style& s) {
            s.set_color(Color::Transparent());
            s.set_outline_color(Color::White());
        });
        layout.get_span("white_red")->adjust_style([](Style& s) {
            s.set_color(Color::White());
            s.set_outline_color(Color::Red());
        });
        layout.update(view);
    };

    Text help_text(mono_font, "[+] thicker outline\n"
                              "[-] thinner outline\n");
    help_text.set_tab_stops({40_vp});
    help_text.set_color(Color(50, 200, 100));
    help_text.set_font_size(3_vp);

    Text info_text(font, fmt::format("Outline radius: {} px", outline_radius));
    info_text.set_color(Color(200, 100, 50));
    info_text.set_font_size(3.5_vp);

    Sprites font_texture(renderer, font.texture(), Color(0, 50, 255));

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
            case Key::Minus:  // -/_ key
            case Key::KeypadSubtract:
                outline_radius -= 0.1f;
                if (outline_radius < 0.0f)
                    outline_radius = 0.0f;
                break;
            case Key::Equal:  // =/+ key
            case Key::KeypadAdd:
                outline_radius += 0.1f;
                break;
            default:
                return;
        }
        text.set_outline_radius(outline_radius);
        text.update(view);
        apply_spans(view);

        info_text.set_string(fmt::format("Outline radius: {} px", outline_radius));
        info_text.update(view);

        view.refresh();
    });

    window.set_size_callback([&](View& view) {
        font.clear_cache();

        help_text.resize(view);
        info_text.resize(view);

        text.set_width(view.viewport_size().x / 2.f);
        text.resize(view);
        apply_spans(view);

        auto tex_size = FramebufferSize{font.texture().size()};
        font_texture.clear();
        font_texture.add_sprite({0, 0, tex_size.x, tex_size.y});
        font_texture.update();
    });

    window.set_update_callback([&](View& view, std::chrono::nanoseconds) {
        text.update(view);
    });

    window.set_draw_callback([&](View& view) {
        help_text.draw(view, {-8.5_vp, -45_vp});
        info_text.draw(view, {-8.5_vp, 45_vp});
        text.draw(view, {-8.5_vp, -20_vp});

        font_texture.draw(view, {-0.5f * view.viewport_size().x + 0.5_vp,
                                 -0.5f * view.viewport_size().y + 0.5_vp});
    });

    window.display();
    return EXIT_SUCCESS;
}
