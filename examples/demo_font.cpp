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

static const char * some_text =
        "One morning, when Gregor Samsa \n"
        "woke from troubled dreams, he found\n"
        "himself transformed in his bed into\n"
        "a horrible vermin. He lay on his\n"
        "armour-like back, and if he lifted\n"
        "his head a little he could see his\n"
        "brown belly, slightly domed and\n"
        "divided by arches into stiff sections.\n"
        "The bedding was hardly able to cover\n"
        "it and seemed ready to slide off any\n"
        "moment.";

int main()
{
    FontFace face;
    if (!face.load_from_file("fonts/Share_Tech_Mono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;

    Font font;
    font.add_face(face);

    Window window;
    window.create({800, 600}, "XCI font demo");

    View view = window.create_view();

    Text text;
    text.set_string(some_text);
    text.set_font(font);
    text.set_size(20);
    text.set_color(Color::White());
    text.draw(view, {-100, -200});

    Sprites font_texture(font.get_texture());
    font_texture.add_sprite({0, 0}, Color::White());
    font_texture.draw(view, {-300, -200});

    window.display();
    return EXIT_SUCCESS;
}
