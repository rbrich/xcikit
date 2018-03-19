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
//
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
    Window window;
    window.create({800, 600}, "XCI layout demo");

    FontFace face;
    if (!face.load_from_file("fonts/Share_Tech_Mono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;

    Font font;
    font.add_face(face);

    Text text;
    text.set_string(sample_text);
    text.set_width(1.33);
    text.set_font(font);
    text.set_size(0.07);
    text.set_color(Color::White());

    window.display([&](View& view){
        text.draw(view, {-0.166, -0.333});

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
