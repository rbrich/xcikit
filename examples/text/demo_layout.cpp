// demo_layout.cpp created on 2018-03-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "graphics/common.h"

#include <xci/text/Font.h>
#include <xci/text/Text.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/Sprites.h>
#include <xci/vfs/Vfs.h>
#include <xci/config.h>
#include <cstdlib>

using namespace xci::text;
using namespace xci::graphics;
using namespace xci::core;

// TODO: * justify
//       * demonstrate setting attributes on a span
// FIXME: comma may land on next line when reflowed

static const char * sample_text =
        "Each paragraph is broken into <c:#AAF>lines</c>. "
        "The lines are further <s:span1>broken into <c:#AAF>words</c></s:span1>, each of which "
        "is shaped and rendered as a run of <c:#AAF>glyphs</c>."
        "\n\n"
        "Each line is bound to a base line, each word is attached to a base point. "
        "To justify the text to a column, the residual space can be uniformly "
        "<s:span2>divided between all words</s:span2> on the line. (This is not yet implemented.)"
        "\n\n"
        "Here is a ligature: in<c:#FAA>fi</c>nity ∞";


int main(int argc, const char* argv[])
{
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};

    setup_window(window, "XCI layout demo", argv);

    Font font {renderer, 512u};
    if (!font.add_face(vfs, "fonts/Lora/Lora[wght].ttf", 0))
        return EXIT_FAILURE;
    if (!font.add_face(vfs, "fonts/Lora/Lora-Italic[wght].ttf", 0))
        return EXIT_FAILURE;

    Font mono_font{renderer};
    if (!mono_font.add_face(vfs, "fonts/ShareTechMono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;

    uint16_t font_weight = 400;

    Text text;
    text.set_markup_string(sample_text);
    text.set_width(66.5_vp);
    text.set_font(font);
    text.set_font_size(4.5_vp);
    text.set_font_style(FontStyle::Italic);
    text.set_font_weight(font_weight);
    text.set_color(Color::White());

    Text help_text(mono_font, "[g] show glyph quads\t[<] align left\n"
                         "[o] show word base points\t[>] align right\n"
                         "[w] show word boxes\t[|] center\n"
                         "[u] show line base lines\n"
                         "[l] show line boxes\n"
                         "[s] show span boxes\n"
                         "[p] show page boxes\n");
    help_text.set_tab_stops({4_vp});
    help_text.set_color(Color(50, 200, 100));
    help_text.set_font_size(3_vp);

    Text help_text_2(mono_font, fmt::format("[+]/[-] Font weight: {}", font_weight));
    help_text_2.set_color(Color(50, 200, 100));
    help_text_2.set_font_size(3_vp);

    Text help_text_3(mono_font, "Resize the window to watch the reflow.");
    help_text_3.set_color(Color(200, 100, 50));
    help_text_3.set_font_size(3_vp);

    Sprites font_texture(renderer, font.texture(), font.sampler(), Color(0, 50, 255));

    View::DebugFlags debug_flags = 0;
    window.set_key_callback([&](View& view, KeyEvent ev) {
        if (ev.action != Action::Press)
            return;
        auto orig_font_weight = font_weight;
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
            case Key::Minus:  // -/_ key
            case Key::KeypadMinus:
                font_weight -= 50;
                if (font_weight < 400)
                    font_weight = 400;
                break;
            case Key::Equal:  // =/+ key
            case Key::KeypadPlus:
                font_weight += 50;
                if (font_weight > 700)
                    font_weight = 700;
                break;
            default:
                return;
        }

        if (font_weight != orig_font_weight) {
            text.set_font_weight(font_weight);
            text.update(view);
            help_text_2.set_string(fmt::format("[+]/[-] Font weight: {}", font_weight));
            help_text_2.update(view);
        }

        view.set_debug_flags(debug_flags);
        view.refresh();
    });

    window.set_size_callback([&](View& view) {
        font.clear_cache();

        help_text.resize(view);
        help_text_2.resize(view);
        help_text_3.resize(view);

        text.set_width(view.viewport_size().x / 2.f);
        text.resize(view);

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
        help_text_2.draw(view, {-8.5_vp, 37.5_vp});
        help_text_3.draw(view, {-8.5_vp, 45_vp});
        text.draw(view, {-8.5_vp, -20_vp});

        font_texture.draw(view, {-0.5f * view.viewport_size().x + 0.5_vp,
                                 -0.5f * view.viewport_size().y + 0.5_vp});
    });

    window.display();
    return EXIT_SUCCESS;
}
