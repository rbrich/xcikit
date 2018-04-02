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
#include <xci/util/format.h>
#include <xci/util/file.h>
#include <cstdlib>

using namespace xci::text;
using namespace xci::graphics;
using namespace xci::util;

int main()
{
    chdir_to_share();

    Window window;
    window.create({800, 600}, "XCI coords demo");

    FontFace face;
    if (!face.load_from_file("fonts/Share_Tech_Mono/ShareTechMono-Regular.ttf", 0))
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
    size_screen.set_color(Color(90, 140, 255));
    Text size_frame("Framebuffer size:  ", font);
    size_frame.set_color(Color(110, 130, 255));
    Text size_font("Font size:         ", font);
    size_font.set_color(Color(70, 150, 255));

    window.display([&](View& view) {
        coords_center.draw(view, {0.0f, 0.0f});
        {
            Vec2f xy = {-0.5f * view.scalable_size().x,
                        -0.5f * view.scalable_size().y};
            coords_tl.set_fixed_string(format("({}, {})", xy.x, xy.y));
            coords_tl.draw(view, {xy.x + 0.1f, xy.y + 0.1f});
        }
        {
            Vec2f xy = {+0.5f * view.scalable_size().x,
                        +0.5f * view.scalable_size().y};
            coords_br.set_fixed_string(format("({}, {})", xy.x, xy.y));
            coords_br.draw(view, {xy.x - 0.4f, xy.y - 0.1f});
        }
        {
            Vec2f xy = {+0.5f * view.scalable_size().x,
                        -0.5f * view.scalable_size().y};
            coords_tr.set_fixed_string(format("({}, {})", xy.x, xy.y));
            coords_tr.draw(view, {xy.x - 0.4f, xy.y + 0.1f});
        }
        {
            Vec2f xy = {-0.5f * view.scalable_size().x,
                        +0.5f * view.scalable_size().y};
            coords_bl.set_fixed_string(format("({}, {})", xy.x, xy.y));
            coords_bl.draw(view, {xy.x + 0.1f, xy.y - 0.1f});
        }

        auto scal = view.scalable_size();
        size_scal.set_fixed_string("Scalable size:     " +
                                   format("{} x {}", scal.x, scal.y) +
                                   "  (1.0 x 1.0)");
        size_scal.draw(view, {-0.4f, -0.5f});

        auto scr = view.screen_size();
        auto pxr = view.screen_ratio();
        size_screen.set_fixed_string("Screen size:       " +
                                     format("{} x {}", scr.x, scr.y) +
                                     "  (" + format("{} x {}", 1/pxr.x, 1/pxr.y) + ")");
        size_screen.draw(view, {-0.4f, -0.4f});

        auto fb = view.framebuffer_size();
        auto pxf = view.framebuffer_ratio();
        size_frame.set_fixed_string("Framebuffer size:  " +
                                    format("{} x {}", fb.x, fb.y) +
                                    "  (" + format("{} x {}", 1/pxf.x, 1/pxf.y) + ")");
        size_frame.draw(view, {-0.4f, -0.3f});

        size_font.set_fixed_string("Font size:         "
                                   + format("{}", font.size()));
        size_font.draw(view, {-0.4f, -0.2f});
    });
    return EXIT_SUCCESS;
}
