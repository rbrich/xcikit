// demo_layout.cpp created on 2018-03-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
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

// TODO: * vertical space between paragraphs
//       * justify
//       * demonstrate setting attributes on a span

static const char * sample_text =
        "Each paragraph is broken into lines. "
        "The lines are further <span1>broken into words</span1>, each of which "
        "is shaped and rendered as a run of glyphs."
        "\n\n"
        "Each line is bound to a base line, each word is attached to a base point. "
        "To justify the text to a column, the residual space can be uniformly "
        "<span2>divided between all words</span2> on the line. (This is not yet implemented.)"
        "\n\n"
        "Here is a ligature: infinity ∞";


int main(int argc, const char* argv[])
{
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};

    setup_window(window, "XCI layout demo", argv);

    Font font {renderer};
    if (!font.add_face(vfs, "fonts/Lora/Lora-VF.ttf", 0))
        return EXIT_FAILURE;
    if (!font.add_face(vfs, "fonts/Lora/Lora-Italic-VF.ttf", 0))
        return EXIT_FAILURE;

    Font mono_font{renderer};
    if (!mono_font.add_face(vfs, "fonts/ShareTechMono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;

    Text text;
    text.set_markup_string(sample_text);
    text.set_width(1.33f);
    text.set_font(font);
    text.set_font_size(0.09f);
    text.set_font_style(FontStyle::Italic);
    text.set_color(Color::White());

    Text help_text(mono_font, "[g] show glyph quads\t[<] align left\n"
                         "[o] show word base points\t[>] align right\n"
                         "[w] show word boxes\t[|] center\n"
                         "[u] show line base lines\n"
                         "[l] show line boxes\n"
                         "[s] show span boxes\n"
                         "[p] show page boxes\n");
    help_text.set_tab_stops({0.8f});
    help_text.set_color(Color(50, 200, 100));
    help_text.set_font_size(0.06f);

    Text help_text_2(font, "Resize the window to see the reflow.");
    help_text_2.set_color(Color(200, 100, 50));
    help_text_2.set_font_size(0.07f);

    Sprites font_texture(renderer, font.texture(), Color(0, 50, 255));

    View::DebugFlags debug_flags = 0;
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
            case Key::G:
                debug_flags ^= (int)View::Debug::GlyphBBox;
                break;
            case Key::O:
                debug_flags ^= (int)View::Debug::WordBasePoint;
                break;
            case Key::W:
                debug_flags ^= (int)View::Debug::WordBBox;
                break;
            case Key::U:
                debug_flags ^= (int)View::Debug::LineBaseLine;
                break;
            case Key::L:
                debug_flags ^= (int)View::Debug::LineBBox;
                break;
            case Key::S:
                debug_flags ^= (int)View::Debug::SpanBBox;
                break;
            case Key::P:
                debug_flags ^= (int)View::Debug::PageBBox;
                break;
            case Key::Comma:
                text.set_alignment(Alignment::Left);
                break;
            case Key::Period:
                text.set_alignment(Alignment::Right);
                break;
            case Key::Backslash:
                text.set_alignment(Alignment::Center);
                break;
            case Key::Equal:
                text.set_alignment(Alignment::Justify);
                break;
            default:
                return;
        }
        view.set_debug_flags(debug_flags);
        view.refresh();
    });

    window.set_size_callback([&](View& view) {
        font.clear_cache();

        help_text.resize(view);
        help_text_2.resize(view);

        text.set_width(view.viewport_size().x / 2.f);
        text.resize(view);

        auto tex_size = view.size_to_viewport(FramebufferSize{font.texture().size()});
        ViewportRect rect = {0, 0, tex_size.x, tex_size.y};
        font_texture.clear();
        font_texture.add_sprite(rect);
        font_texture.update();
    });

    window.set_update_callback([&](View& view, std::chrono::nanoseconds) {
        text.update(view);
    });

    window.set_draw_callback([&](View& view) {
        help_text.draw(view, {-0.17f, -0.9f});
        help_text_2.draw(view, {-0.17f, 0.9f});
        text.draw(view, {-0.17f, -0.3f});

        font_texture.draw(view, {-0.5f * view.viewport_size().x + 0.01f,
                                 -0.5f * view.viewport_size().y + 0.01f});
    });

    window.display();
    return EXIT_SUCCESS;
}
