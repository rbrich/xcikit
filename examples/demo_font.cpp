// demo_font.cpp created on 2018-03-02, part of XCI toolkit
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

#include <xci/text/FontLibrary.h>
#include <xci/text/FontFace.h>
#include <xci/text/Font.h>
#include <xci/text/Text.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/Sprites.h>
#include <xci/util/file.h>
#include <cstdlib>

using namespace xci::text;
using namespace xci::graphics;

// sample text with forced newlines
static const char * sample_text = R"SAMPLE(
Ty třepotné, smavé hvězdičky{br}
tak čiperně na mne hledí -{br}
ach prosím vás, je to pravda vše,{br}
co lidé prý o vás vědí?{br}
{br}
Že maličké vy prý hvězdičky{br}
jste obrovská samá těla -{br}
a od jedné k druhé prý sto let{br}
a k některé věčnost celá?{br}
)SAMPLE";

int main()
{
    xci::util::chdir_to_share();

    Window window;
    window.create({800, 600}, "XCI font demo");

    FontFace face;
    if (!face.load_from_file("fonts/Enriqueta/Enriqueta-Regular.ttf", 0))
        return EXIT_FAILURE;

    Font font;
    font.add_face(face);

    Text text;
    text.set_string(sample_text);
    text.set_font(font);
    text.set_size(0.08);
    text.set_color(Color::White());

    window.display([&](View& view){
        text.draw(view, {0.5f * view.scalable_size().x - 2.0f, -0.5f});

        auto& tex = font.get_texture();
        Sprites font_texture(tex);
        Rect_f rect = {0, 0,
                       tex.size().x * view.framebuffer_ratio().x,
                       tex.size().y * view.framebuffer_ratio().y};
        font_texture.add_sprite(rect);
        font_texture.draw(view, {-0.5f * view.scalable_size().x + 0.01f, -0.5f * rect.h});
    });
    return EXIT_SUCCESS;
}
