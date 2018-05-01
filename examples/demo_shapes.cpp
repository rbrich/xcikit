// demo_rectangles.cpp created on 2018-03-19, part of XCI toolkit
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

#include <xci/text/FontFace.h>
#include <xci/text/Font.h>
#include <xci/text/Text.h>
#include <xci/graphics/Window.h>
#include <xci/graphics/Shape.h>
#include <xci/util/file.h>
#include <cstdlib>

using namespace xci::text;
using namespace xci::graphics;

int main()
{
    xci::util::chdir_to_share();

    Window& window = Window::default_window();
    window.create({800, 600}, "XCI shapes demo");

    FontFace face;
    if (!face.load_from_file("fonts/ShareTechMono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;
    Font font;
    font.add_face(face);

    Text shapes_help("[r] rectangles{br}"
                     "[o] rounded rectangles{br}"
                     "[e] ellipses{br}"
                     "[l] lines", font);
    shapes_help.set_color(Color(200, 100, 50));
    Text option_help("[a] antialiasing{br}"
                     "[s] softness", font);
    option_help.set_color(Color(200, 100, 50));

    // normally, the border scales with viewport size
    Shape shapes(Color(0, 0, 40, 128), Color(180, 180, 0));
    // using View::screen_ratio, we can set constant border width, in screen pixels
    Shape shapes_px(Color(40, 40, 0, 128), Color(255, 255, 0));

    std::function<void(Shape&, const Rect_f&, float)>
    add_shape_fn = [](Shape& shape, const Rect_f& rect, float th) {
        shape.add_rectangle(rect, th);
    };

    float antialiasing = 0;
    float softness = 0;

    window.set_key_callback([&](View& view, KeyEvent ev){
        switch (ev.key) {
            case Key::R:
                add_shape_fn = [](Shape& shape, const Rect_f& rect, float th) {
                    shape.add_rectangle(rect, th);
                };
                break;
            case Key::O:
                add_shape_fn = [](Shape& shape, const Rect_f& rect, float th) {
                    shape.add_rounded_rectangle(rect, 0.05, th);
                };
                break;
            case Key::E:
                add_shape_fn = [](Shape& shape, const Rect_f& rect, float th) {
                    shape.add_ellipse(rect, th);
                };
                break;
            case Key::L:
                add_shape_fn = [](Shape& shape, const Rect_f& rect, float th) {
                    auto l = rect.left();
                    auto t = rect.top();
                    auto r = rect.right();
                    auto b = rect.bottom();
                    auto w2 = rect.w / 2;
                    auto h2 = rect.h / 2;
                    auto w4 = rect.w / 4;
                    auto h4 = rect.h / 4;
                    auto c = rect.center();
                    shape.add_line_slice({l, t, w2, h2}, {l, t+h4}, {c.x, t}, th);
                    shape.add_line_slice({c.x, t, w2, h2}, {r-w4, t}, {r, c.y}, th);
                    shape.add_line_slice({c.x, c.y, w2, h2}, {r, b-h4}, {c.x, b}, th);
                    shape.add_line_slice({l, c.y, w2, h2}, {l+w4, b}, {l, c.y}, th);
                };
                break;
            case Key::A:
                antialiasing = (antialiasing == 0) ? 2 : 0;
                shapes.set_antialiasing(antialiasing);
                shapes_px.set_antialiasing(antialiasing);
                break;
            case Key::S:
                softness = (softness == 0) ? 1 : 0;
                shapes.set_softness(softness);
                shapes_px.set_softness(softness);
                break;
            default:
                break;
        }
        view.refresh();
    });

    window.set_draw_callback([&](View& view) {
        Vec2f vs = view.scalable_size();
        shapes_help.draw(view, {-vs.x/2 + 0.1f, -vs.y/2 + 0.1f});
        option_help.draw(view, {vs.x/2 - 0.5f, -vs.y/2 + 0.1f});

        add_shape_fn(shapes, {-1, -0.6f, 2, 1.2f}, 0.05);
        add_shape_fn(shapes, {-0.6f, -0.8f, 1.2f, 1.6f}, 0.02);
        shapes.draw(view, {0, 0});
        shapes.clear();

        auto pxr = view.screen_ratio().x;
        add_shape_fn(shapes_px, {0.0f, 0.0f, 0.5f, 0.5f}, 1 * pxr);
        add_shape_fn(shapes_px, {0.1f, 0.1f, 0.5f, 0.5f}, 2 * pxr);
        add_shape_fn(shapes_px, {0.2f, 0.2f, 0.5f, 0.5f}, 3 * pxr);
        add_shape_fn(shapes_px, {0.3f, 0.3f, 0.5f, 0.5f}, 4 * pxr);
        add_shape_fn(shapes_px, {0.4f, 0.4f, 0.5f, 0.5f}, 5 * pxr);
        shapes_px.draw(view, {-0.45f, -0.45f});
        shapes_px.clear();
    });

    window.display();
    return EXIT_SUCCESS;
}
