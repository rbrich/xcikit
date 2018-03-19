// demo_font.cpp created on 2018-03-02, part of XCI toolkit
//
// Set WORKDIR to project root.

#include <xci/text/FontLibrary.h>
#include <xci/text/FontFace.h>
#include <xci/text/Font.h>
#include <xci/text/Text.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/Sprites.h>

using namespace xci::text;
using namespace xci::graphics;

// sample text with forced newlines
static const char * sample_text =
        "One morning, when Gregor Samsa\n\n"
        "woke from troubled dreams, he found\n\n"
        "himself transformed in his bed into\n\n"
        "a horrible vermin. He lay on his\n\n"
        "armour-like back, and if he lifted\n\n"
        "his head a little he could see his\n\n"
        "brown belly, slightly domed and\n\n"
        "divided by arches into stiff sections.\n\n"
        "The bedding was hardly able to cover\n\n"
        "it and seemed ready to slide off any\n\n"
        "moment.";

int main()
{
    Window window;
    window.create({800, 600}, "XCI font demo");

    FontFace face;
    if (!face.load_from_file("fonts/Share_Tech_Mono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;

    Font font;
    font.add_face(face);

    Text text;
    text.set_string(sample_text);
    text.set_font(font);
    text.set_size(0.08);
    text.set_color(Color::White());

    window.display([&](View& view){
        text.draw(view, {0.5f * view.size().x - 2.0f, -0.5f});

        auto& tex = font.get_texture();
        Sprites font_texture(tex);
        Rect_f rect = {0, 0,
                       tex.size().x / view.pixel_ratio().x,
                       tex.size().y / view.pixel_ratio().y};
        font_texture.add_sprite(rect);
        font_texture.draw(view, {-0.5f * view.size().x + 0.01f, -0.5f * rect.h});
    });
    return EXIT_SUCCESS;
}
