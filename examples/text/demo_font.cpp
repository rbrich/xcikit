// demo_font.cpp created on 2018-03-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "graphics/common.h"

#include <xci/text/Font.h>
#include <xci/text/Text.h>
#include <xci/graphics/Renderer.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/Sprites.h>
#include <xci/graphics/shape/Rectangle.h>
#include <xci/core/Vfs.h>
#include <xci/config.h>
#include <cstdlib>

using namespace xci::text;
using namespace xci::graphics;
using namespace xci::core;

// sample text with forced newlines
// source: http://www.columbia.edu/~fdc/utf8/index.html
static const char * sample_text = R"SAMPLE(
Vitrum edere possum; mihi non nocet.<br>
Posso mangiare il vetro e non mi fa male.<br>
Je peux manger du verre, ça ne me fait pas mal.<br>
Puedo comer vidrio, no me hace daño.<br>
Posso comer vidro, não me faz mal.<br>
Mi kian niam glas han i neba hot mi.<br>
Ich kann Glas essen, ohne mir zu schaden.<br>
Mogę jeść szkło i mi nie szkodzi.<br>
Meg tudom enni az üveget, nem lesz tőle bajom.<br>
Pot să mănânc sticlă și ea nu mă rănește.<br>
Eg kan eta glas utan å skada meg.<br>
Ik kan glas eten, het doet mĳ geen kwaad.<br>
)SAMPLE";

int main(int argc, const char* argv[])
{
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};

    setup_window(window, "XCI font demo", argv);

    Font font {renderer};
    if (!font.add_face(vfs, "fonts/Enriqueta/Enriqueta-Regular.ttf", 0))
        return EXIT_FAILURE;
    if (!font.add_face(vfs, "fonts/Enriqueta/Enriqueta-Bold.ttf", 0))
        return EXIT_FAILURE;

    Font emoji_font {renderer, 1024u};
    if (!emoji_font.add_face(vfs, "fonts/Noto/NotoColorEmoji.ttf", 0))
        return EXIT_FAILURE;

    static constexpr ViewportUnits text_font_size = 5_vp;
    Text text;
    text.set_markup_string(sample_text);
    text.set_font(font);
    text.set_font_size(text_font_size);
    text.set_color(Color::White());

    Text emoji;
    emoji.set_fixed_string("🥛🍸🥃🥂🍷🍹⚗️🧂");
    emoji.set_font(emoji_font);
    emoji.set_font_size(10_vp);

    static constexpr auto help_color_normal = Color(200, 100, 50);
    static constexpr auto help_color_highlight = Color(255, 170, 120);
    Text help_text(font, "<s:smooth><b>[s]</b> smooth scaling</s:smooth> <tab>"
                         "<s:font><b>[f]</b> font scaling</s:font><br>"
                         "(Resize window to observe the scaling effect.)", TextFormat::Markup);
    help_text.set_color(help_color_normal);
    help_text.set_font_size(5_vp);

    auto help_highlight = [&help_text, &text](const View& view) {
        bool smooth = text.layout().default_style().allow_scale();
        help_text.layout().get_span("smooth")->adjust_style([smooth](Style& s) {
            s.set_color(smooth ? help_color_highlight : help_color_normal);
        });
        help_text.layout().get_span("font")->adjust_style([smooth](Style& s) {
            s.set_color(!smooth ? help_color_highlight : help_color_normal);
        });
        help_text.layout().update(view);
    };

    Sprites font_texture(renderer, font.texture(), Color::Blue());
    Sprites emoji_font_texture(renderer, emoji_font.texture(), Color::Blue());

    Rectangle rects(renderer);

    FramebufferPixels emoji_offset = 0.f;

    window.set_size_callback([&](View& view) {
        text.resize(view);
        emoji.resize(view);
        help_text.resize(view);
        help_highlight(view);

        auto tex_size = FramebufferSize{font.texture().size()};
        FramebufferRect rect = {0, 0, tex_size.x, tex_size.y};
        emoji_offset = rect.size().y + 0.04f;

        font_texture.clear();
        font_texture.add_sprite(rect);
        font_texture.update();

        emoji_font_texture.clear();
        emoji_font_texture.add_sprite(rect);
        emoji_font_texture.update();

        auto enl_rect = rect.enlarged(0.01f);
        rects.clear();
        rects.add_rectangle(enl_rect, view.px_to_fb(1_px));
        rects.add_rectangle(enl_rect.moved({0, emoji_offset}), view.px_to_fb(1_px));
        rects.update(Color::Transparent(), Color::Grey());
    });

    window.set_draw_callback([&](View& view) {
        auto vs = view.viewport_size();
        ViewportCoords font_pos {-0.5f * vs.x + 0.5_vp, -0.5f * vs.y + 0.5_vp};
        rects.draw(view, font_pos);
        font_texture.draw(view, font_pos);
        font_pos.y += view.fb_to_vp(emoji_offset);
        emoji_font_texture.draw(view, font_pos);

        // font texture width
        auto fw = view.fb_to_vp(font.texture().size().x);
        auto ew = view.fb_to_vp(emoji.layout().bbox().w);
        auto tx = -vs.x * 0.5f + fw  // the font box right edge
                + (vs.x - fw - ew) / 2;  // half of empty space left around text
        text.draw(view, {tx, -27.5_vp});
        emoji.draw(view, {tx, -35_vp});
        help_text.draw(view, {tx, 35_vp});
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
            case Key::S:
                text.set_font_size(text_font_size, true);
                text.update(view);
                help_highlight(view);
                view.refresh();
                break;
            case Key::F:
                text.set_font_size(text_font_size, false);
                text.update(view);
                help_highlight(view);
                view.refresh();
                break;
            default:
                break;
        }
    });

    window.set_refresh_mode(RefreshMode::OnDemand);
    window.display();
    return EXIT_SUCCESS;
}
