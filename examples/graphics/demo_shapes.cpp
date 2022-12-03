// demo_rectangles.cpp created on 2018-03-19 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "common.h"

#include <xci/text/Font.h>
#include <xci/text/Text.h>
#include <xci/graphics/Shape.h>
#include <xci/core/Vfs.h>
#include <xci/config.h>
#include <cstdlib>

using namespace xci::text;

int main(int argc, const char* argv[])
{
    Vfs vfs;
    if (!vfs.mount(XCI_SHARE))
        return EXIT_FAILURE;

    Renderer renderer {vfs};
    Window window {renderer};
    setup_window(window, "XCI shapes demo", argv);

    Font font {renderer};
    if (!font.add_face(vfs, "fonts/ShareTechMono/ShareTechMono-Regular.ttf", 0))
        return EXIT_FAILURE;

    Text shapes_help(font, "[r] rectangles\n"
                           "[o] rounded rectangles\n"
                           "[e] ellipses\n"
                           "[l] lines\n");
    shapes_help.set_color(Color(200, 100, 50));
    Text option_help(font, "[a] antialiasing\n"
                           "[s] softness\n");
    option_help.set_color(Color(200, 100, 50));

    Shape shapes[7] {Shape{renderer}, Shape{renderer}, Shape{renderer},
                     Shape{renderer}, Shape{renderer}, Shape{renderer},
                     Shape{renderer} };
    int antialiasing = 0;
    int softness = 0;

    std::function add_shape_fn = [](Shape& shape, const FramebufferRect& rect, FramebufferPixels th)
    {
        shape.add_rectangle(rect, th);
    };

    auto set_shape_attr = [&](Shape& shape) {
        if (&shape == &shapes[0] || &shape == &shapes[1]) {
            shape.set_fill_color(Color(0, 0, 40, 128));
            shape.set_outline_color(Color(180, 180, 0));
        } else {
            shape.set_fill_color(Color(40, 40, 0, 128));
            shape.set_outline_color(Color(255, 255, 0));
        }
        shape.set_antialiasing(antialiasing);
        shape.set_softness(softness);
    };

    auto recreate_shapes = [&](View& view) {
        view.finish_draw();

        for (Shape& shape : shapes) {
            shape.clear();
            set_shape_attr(shape);
        }

        // Border scaled with viewport size
        add_shape_fn(shapes[0], view.vp_to_fb({-50_vp, -30_vp, 100_vp, 60_vp}), view.vp_to_fb(2.5_vp));
        add_shape_fn(shapes[1], view.vp_to_fb({-30_vp, -40_vp, 60_vp, 80_vp}), view.vp_to_fb(1_vp));

        // Constant border width, in screen pixels
        add_shape_fn(shapes[2], view.vp_to_fb({ 0_vp,  0_vp, 25_vp, 25_vp}), view.px_to_fb(1_px));
        add_shape_fn(shapes[3], view.vp_to_fb({ 5_vp,  5_vp, 25_vp, 25_vp}), view.px_to_fb(2_px));
        add_shape_fn(shapes[4], view.vp_to_fb({10_vp, 10_vp, 25_vp, 25_vp}), view.px_to_fb(3_px));
        add_shape_fn(shapes[5], view.vp_to_fb({15_vp, 15_vp, 25_vp, 25_vp}), view.px_to_fb(4_px));
        add_shape_fn(shapes[6], view.vp_to_fb({20_vp, 20_vp, 25_vp, 25_vp}), view.px_to_fb(5_px));

        for (Shape& shape : shapes)
            shape.update();
    };

    window.set_key_callback([&](View& view, KeyEvent ev){
        if (ev.action != Action::Press)
            return;
        switch (ev.key) {
            case Key::Escape:
                window.close();
                break;
            case Key::F:
            case Key::F11:
                window.toggle_fullscreen();
                break;
            case Key::R:
                add_shape_fn = [](Shape& shape, const FramebufferRect& rect, FramebufferPixels th) {
                    shape.add_rectangle(rect, th);
                };
                break;
            case Key::O:
                add_shape_fn = [&](Shape& shape, const FramebufferRect& rect, FramebufferPixels th) {
                    shape.add_rounded_rectangle(rect, view.vp_to_fb(2.5_vp), th);
                };
                break;
            case Key::E:
                add_shape_fn = [](Shape& shape, const FramebufferRect& rect, FramebufferPixels th) {
                    shape.add_ellipse(rect, th);
                };
                break;
            case Key::L:
                add_shape_fn = [](Shape& shape, const FramebufferRect& rect, FramebufferPixels th) {
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
                break;
            case Key::S:
                softness = (softness == 0) ? 1 : 0;
                break;
            default:
                return;
        }
        recreate_shapes(view);
        view.refresh();
    });

    window.set_size_callback([&](View& view) {
        shapes_help.resize(view);
        option_help.resize(view);
        recreate_shapes(view);
    });

    window.set_draw_callback([&](View& view) {
        auto vs = view.viewport_size();
        shapes_help.draw(view, {-vs.x / 2 + 5_vp, -vs.y / 2 + 5_vp});
        option_help.draw(view, {vs.x / 2 - 25_vp, -vs.y / 2 + 5_vp});

        shapes[0].draw(view, {0_vp, 0_vp});
        shapes[1].draw(view, {0_vp, 0_vp});

        for (size_t i = 2; i <= 6; i++)
            shapes[i].draw(view, {-22.5_vp, -22.5_vp});
    });

    window.set_refresh_mode(RefreshMode::OnDemand);
    window.display();
    return EXIT_SUCCESS;
}
