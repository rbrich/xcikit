// demo_layout.cpp created on 2018-03-10, part of XCI toolkit
// Copyright 2018 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// demo_font.cpp created on 2018-03-02, part of XCI toolkit
//
// Set WORKDIR to project root.

#include <xci/text/FontLibrary.h>
#include <xci/text/FontFace.h>
#include <xci/text/Font.h>
#include <xci/text/Text.h>
#include <xci/text/Layout.h>
#include <xci/text/Markup.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/Sprites.h>

using namespace xci::text;
using namespace xci::graphics;

static const char * sample_text =
        "One morning, when Gregor Samsa "
        "woke from troubled dreams, he found "
        "himself transformed in his bed into "
        "a horrible vermin. He lay on his "
        "armour-like back, and if he lifted "
        "his head a little he could see his "
        "brown belly, slightly domed and "
        "divided by arches into stiff sections. "
        "The bedding was hardly able to cover "
        "it and seemed ready to slide off any "
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

    Layout layout;
    layout.set_width(400);
    layout.set_font(font);
    layout.set_size(20);
    layout.set_color(Color::White());

    Markup markup(layout);
    markup.parse(sample_text);

    window.display([&](View& view){
        layout.draw(view, {-100, -200});

        Sprites font_texture(font.get_texture());
        font_texture.add_sprite({0, 0}, Color::White());
        font_texture.draw(view, {-300, -200});
    });
    return EXIT_SUCCESS;
}
