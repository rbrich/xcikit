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

static const char * sample_text =
        "One morning, when Gregor Samsa "
        "woke from troubled dreams, he found "
        "himself transformed in his bed into "
        "a horrible vermin. {+span1}He lay on his "
        "armour-like back{-span1}, and if he lifted "
        "his head a little he could see his "
        "brown belly, {+span2}slightly domed{-span2} and "
        "divided by arches into stiff sections. "
        "The bedding was hardly able to cover "
        "it and seemed ready to slide off any "
        "moment.";

int main()
{
    xci::util::chdir_to_share();

    Window& window = Window::default_window();
    window.create({800, 600}, "XCI layout demo");

    auto face = std::make_unique<FontFace>();
    if (!face->load_from_file("fonts/ShareTechMono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;

    Font font;
    font.add_face(std::move(face));

    Text text;
    text.set_string(sample_text);
    text.set_width(1.33);
    text.set_font(font);
    text.set_size(0.07);
    text.set_color(Color::White());

    Text help_text("[c] show character quads\n\n"
                   "[o] show word base points\n\n"
                   "[w] show word boxes\n\n"
                   "[u] show line base lines\n\n"
                   "[l] show line boxes\n\n"
                   "[s] show span boxes\n\n"
                   "[p] show page boxes", font);
    help_text.set_color(Color(50, 200, 100));

    View::DebugFlags debug_flags = 0;
    window.set_key_callback([&](View& view, KeyEvent ev){
        if (ev.action != Action::Press)
            return;
        switch (ev.key) {
            case Key::C:
                debug_flags ^= (int)View::Debug::GlyphBBox;
                view.set_debug_flags(debug_flags);
                break;
            case Key::O:
                debug_flags ^= (int)View::Debug::WordBasePoint;
                view.set_debug_flags(debug_flags);
                break;
            case Key::W:
                debug_flags ^= (int)View::Debug::WordBBox;
                view.set_debug_flags(debug_flags);
                break;
            case Key::U:
                debug_flags ^= (int)View::Debug::LineBaseLine;
                view.set_debug_flags(debug_flags);
                break;
            case Key::L:
                debug_flags ^= (int)View::Debug::LineBBox;
                view.set_debug_flags(debug_flags);
                break;
            case Key::S:
                debug_flags ^= (int)View::Debug::SpanBBox;
                view.set_debug_flags(debug_flags);
                break;
            case Key::P:
                debug_flags ^= (int)View::Debug::PageBBox;
                view.set_debug_flags(debug_flags);
                break;
            default:
                break;
        }
        view.refresh();
    });

    window.set_draw_callback([&](View& view) {
        help_text.resize_draw(view, {-0.17f, -0.9f});
        text.resize_draw(view, {-0.17f, -0.3f});

        auto& tex = font.get_texture();
        Sprites font_texture(tex);
        Rect_f rect = {0, 0,
                       tex->size().x * view.framebuffer_ratio().x,
                       tex->size().y * view.framebuffer_ratio().y};
        font_texture.add_sprite(rect);
        font_texture.draw(view, {-0.5f * view.scalable_size().x + 0.01f,
                                 -0.5f * rect.h});
    });

    window.display();
    return EXIT_SUCCESS;
}
