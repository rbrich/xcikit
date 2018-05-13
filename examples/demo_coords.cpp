// demo_coords.cpp created on 2018-03-18, part of XCI toolkit
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
#include <xci/graphics/Shape.h>
#include <xci/graphics/Color.h>
#include <xci/util/format.h>
#include <xci/util/file.h>
#include <cstdlib>

using namespace xci::text;
using namespace xci::graphics;
using namespace xci::util;

int main()
{
    chdir_to_share();

    Window& window = Window::default_window();
    window.create({800, 600}, "XCI coords demo");

    FontFace face;
    if (!face.load_from_file("fonts/ShareTechMono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;
    Font font;
    font.add_face(face);

    Text coords_center("(0, 0)", font);
    Text coords_tl("(-, -)", font);
    Text coords_br("(-, -)", font);
    Text coords_tr("(-, -)", font);
    Text coords_bl("(-, -)", font);
    Text size_scal("View size:         ", font);
    size_scal.set_color(Color(130, 120, 255));
    Text size_screen("Screen size:       ", font);
    size_screen.set_color(Color(110, 130, 255));
    Text size_frame("Framebuffer size:  ", font);
    size_frame.set_color(Color(90, 140, 255));
    Text size_font("Font size:         ", font);
    size_font.set_color(Color(70, 150, 255));
    Text mouse_pos("Mouse position:    ", font);
    mouse_pos.set_color(Color(255, 150, 50));

    Shape unit_square(Color::Transparent(), Color(0.7, 0.7, 0.7));

    window.set_size_callback([&](View& view) {
        Vec2f vs = view.scalable_size();
        coords_tl.set_fixed_string(format("({}, {})", -0.5f * vs.x, -0.5f * vs.y));
        coords_br.set_fixed_string(format("({}, {})", +0.5f * vs.x, +0.5f * vs.y));
        coords_tr.set_fixed_string(format("({}, {})", +0.5f * vs.x, -0.5f * vs.y));
        coords_bl.set_fixed_string(format("({}, {})", -0.5f * vs.x, +0.5f * vs.y));

        size_scal.set_fixed_string("Scalable size:     " +
                                   format("{} x {}", vs.x, vs.y) +
                                   "  (1.0 x 1.0)");

        auto ps = view.screen_size();
        auto pr = view.screen_ratio();
        size_screen.set_fixed_string("Screen size:       " +
                                     format("{} x {}", ps.x, ps.y) +
                                     "  (" + format("{} x {}", 1/pr.x, 1/pr.y) + ")");

        auto fs = view.framebuffer_size();
        auto fr = view.framebuffer_ratio();
        size_frame.set_fixed_string("Framebuffer size:  " +
                                    format("{} x {}", fs.x, fs.y) +
                                    "  (" + format("{} x {}", 1/fr.x, 1/fr.y) + ")");

        unit_square.clear();
        unit_square.add_rectangle({-1, -1, 2, 2}, pr.y);
    });

    window.set_draw_callback([&](View& view) {
        unit_square.draw(view, {0,0});
        coords_center.draw(view, {0.0f, 0.0f});
        Vec2f vs = view.scalable_size();
        coords_tl.draw(view, {-0.5f * vs.x + 0.1f, -0.5f * vs.y + 0.1f});
        coords_br.draw(view, {+0.5f * vs.x - 0.4f, +0.5f * vs.y - 0.1f});
        coords_tr.draw(view, {+0.5f * vs.x - 0.4f, -0.5f * vs.y + 0.1f});
        coords_bl.draw(view, {-0.5f * vs.x + 0.1f, +0.5f * vs.y - 0.1f});
        size_scal.draw(view, {-0.4f, -0.5f});
        size_screen.draw(view, {-0.4f, -0.4f});
        size_frame.draw(view, {-0.4f, -0.3f});
        size_font.set_fixed_string("Font size:         " +
                                   format("{}", font.size()));
        size_font.draw(view, {-0.4f, -0.2f});
        mouse_pos.draw(view, {-0.4f, 0.2f});
    });

    window.set_mouse_position_callback([&](View& view, const MousePosEvent& ev) {
        mouse_pos.set_fixed_string("Mouse position:    " +
                                   format("({}, {})", ev.pos.x, ev.pos.y));
        view.refresh();
    });

    window.display();
    return EXIT_SUCCESS;
}
