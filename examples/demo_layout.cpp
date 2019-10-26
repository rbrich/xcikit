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
    Vfs vfs;
    vfs.mount(XCI_SHARE_DIR);

    Window& window = Window::default_instance();
    window.create({800, 600}, "XCI layout demo");

    Font font;
    if (!font.add_face(vfs, "fonts/ShareTechMono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;

    Text text;
    text.set_markup_string(sample_text);
    text.set_width(1.33);
    text.set_font(font);
    text.set_font_size(0.07);
    text.set_color(Color::White());

    Text help_text(font, "[c] show character quads\n"
                         "[o] show word base points\n"
                         "[w] show word boxes\n"
                         "[u] show line base lines\n"
                         "[l] show line boxes\n"
                         "[s] show span boxes\n"
                         "[p] show page boxes\n");
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
        help_text.draw(view, {-0.17f, -0.9f});
        text.draw(view, {-0.17f, -0.3f});

        auto& tex = font.get_texture();
        auto tex_size = view.size_to_viewport(FramebufferSize{tex->size()});
        Sprites font_texture(tex);
        ViewportRect rect = {0, 0, tex_size.x, tex_size.y};
        font_texture.add_sprite(rect);
        font_texture.draw(view, {-0.5f * view.viewport_size().x + 0.01f,
                                 -0.5f * rect.h});
    });

    window.display();
    return EXIT_SUCCESS;
}
