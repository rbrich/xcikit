// demo_font.cpp created on 2018-03-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/text/Font.h>
#include <xci/text/Text.h>
#include <xci/graphics/Renderer.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/Sprites.h>
#include <xci/core/Vfs.h>
#include <xci/config.h>
#include <cstdlib>

using namespace xci::text;
using namespace xci::graphics;
using namespace xci::core;

// sample text with forced newlines
// source: http://www.columbia.edu/~fdc/utf8/index.html
static const char * sample_text = R"SAMPLE(
Vitrum edere possum; mihi non nocet.{br}
Posso mangiare il vetro e non mi fa male.{br}
Je peux manger du verre, ça ne me fait pas mal.{br}
Puedo comer vidrio, no me hace daño.{br}
Posso comer vidro, não me faz mal.{br}
Mi kian niam glas han i neba hot mi.{br}
Ich kann Glas essen, ohne mir zu schaden.{br}
Mogę jeść szkło i mi nie szkodzi.{br}
Meg tudom enni az üveget, nem lesz tőle bajom.{br}
Pot să mănânc sticlă și ea nu mă rănește.{br}
Eg kan eta glas utan å skada meg.{br}
Ik kan glas eten, het doet mĳ geen kwaad.{br}
)SAMPLE";

int main()
{
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};
    window.create({800, 600}, "XCI font demo");

    Font font {renderer};
    if (!font.add_face(vfs, "fonts/Enriqueta/Enriqueta-Regular.ttf", 0))
        return EXIT_FAILURE;

    Text text;
    text.set_markup_string(sample_text);
    text.set_font(font);
    text.set_font_size(0.08f);
    text.set_color(Color::White());

    Sprites font_texture(renderer, font.texture(), Color::Blue());

    window.set_size_callback([&](View& view) {
        text.resize(view);

        auto tex_size = view.size_to_viewport(FramebufferSize{font.texture().size()});
        ViewportRect rect = {0, 0, tex_size.x, tex_size.y};
        font_texture.clear();
        font_texture.add_sprite(rect);
        font_texture.update();
    });

    window.set_draw_callback([&](View& view) {
        auto vs = view.viewport_size();
        text.draw(view, {0.5f * vs.x - 1.9f, -0.55f});
        font_texture.draw(view, {-0.5f * vs.x + 0.01f, -0.5f});
    });

    window.set_refresh_mode(RefreshMode::OnDemand);
    window.display();
    return EXIT_SUCCESS;
}
