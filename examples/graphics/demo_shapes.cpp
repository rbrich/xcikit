// demo_rectangles.cpp created on 2018-03-19 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "common.h"

#include <xci/text/Font.h>
#include <xci/text/Text.h>
#include <xci/graphics/shape/Rectangle.h>
#include <xci/graphics/shape/RoundedRectangle.h>
#include <xci/graphics/shape/Ellipse.h>
#include <xci/graphics/shape/Line.h>
#include <xci/graphics/shape/Polygon.h>
#include <xci/core/Vfs.h>
#include <xci/config.h>
#include <variant>
#include <array>
#include <cstdlib>

using namespace xci::text;


class VariantShape {
public:
    VariantShape(Renderer& renderer) : m_shape(std::in_place_type<Rectangle>, renderer) {}

    template <typename T>
    void switch_type(Renderer& renderer) {
        m_shape.emplace<T>(renderer);
    }

    void add_shape(View& view, const ViewportRect& vp_rect, FramebufferPixels th) {
        std::visit([&view, rect = view.vp_to_fb(vp_rect), th] (auto& shape) {
            using T = std::decay_t<decltype(shape)>;
            if constexpr (std::is_same_v<T, Rectangle>) {
                shape.add_rectangle(rect, th);
            } else if constexpr (std::is_same_v<T, RoundedRectangle>) {
                shape.add_rounded_rectangle(rect, view.vp_to_fb(2.5_vp), th);
            } else if constexpr (std::is_same_v<T, Ellipse>) {
                shape.add_ellipse(rect, th);
            } else if constexpr (std::is_same_v<T, Line>) {
                const auto l = rect.left();
                const auto t = rect.top();
                const auto r = rect.right();
                const auto b = rect.bottom();
                const auto w2 = rect.w / 2;
                const auto h2 = rect.h / 2;
                const auto w4 = rect.w / 4;
                const auto h4 = rect.h / 4;
                const auto c = rect.center();
                shape.add_line_slice({l, t, w2, h2}, {l, t+h4}, {c.x, t}, th);
                shape.add_line_slice({c.x, t, w2, h2}, {r-w4, t}, {r, c.y}, th);
                shape.add_line_slice({c.x, c.y, w2, h2}, {r, b-h4}, {c.x, b}, th);
                shape.add_line_slice({l, c.y, w2, h2}, {l+w4, b}, {l, c.y}, th);
            } else if constexpr (std::is_same_v<T, Polygon>) {
                constexpr int edges = 14;
                const float angle = 2 * std::acos(-1) / edges;
                const auto center = rect.center();
                std::vector<FramebufferCoords> vertices;
                for (int i = 0; i <= edges; i++) {
                    vertices.emplace_back(center + ((0.3 + 0.2 * (i % 2)) * rect.size()).rotate(-angle * i));
                }
                shape.add_polygon(center, vertices, th);
            }
        }, m_shape);
    }

    void clear() {
        std::visit([](auto& shape) {
            shape.clear();
        }, m_shape);
    }

    void update(Color fill_color, Color outline_color,
                float softness, float antialiasing)
    {
        std::visit([=](auto& shape) {
            shape.update(fill_color, outline_color, softness, antialiasing);
        }, m_shape);
    }

    void draw(View& view, VariCoords pos) {
        std::visit([&view, pos](auto& shape) {
            shape.draw(view, pos);
        }, m_shape);
    }

private:
    std::variant<Rectangle, RoundedRectangle, Ellipse, Line, Polygon> m_shape;
};


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
                           "[l] lines\n"
                           "[p] polygons\n");
    shapes_help.set_color(Color(200, 100, 50));
    Text option_help(font, "[a] antialiasing\n"
                           "[s] softness\n");
    option_help.set_color(Color(200, 100, 50));

    std::array<VariantShape, 7> shapes { renderer, renderer, renderer, renderer, renderer, renderer, renderer };
    int antialiasing = 0;
    int softness = 0;

    auto recreate_shapes = [&](View& view) {
        view.finish_draw();

        for (VariantShape& shape : shapes) {
            shape.clear();
        }

        // Border scaled with viewport size
        shapes[0].add_shape(view, {-50_vp, -30_vp, 100_vp, 60_vp}, view.vp_to_fb(2.5_vp));
        shapes[1].add_shape(view, {-30_vp, -40_vp, 60_vp, 80_vp}, view.vp_to_fb(1_vp));

        // Constant border width, in screen pixels
        shapes[2].add_shape(view, { 0_vp,  0_vp, 25_vp, 25_vp}, view.px_to_fb(1_px));
        shapes[3].add_shape(view, { 5_vp,  5_vp, 25_vp, 25_vp}, view.px_to_fb(2_px));
        shapes[4].add_shape(view, {10_vp, 10_vp, 25_vp, 25_vp}, view.px_to_fb(3_px));
        shapes[5].add_shape(view, {15_vp, 15_vp, 25_vp, 25_vp}, view.px_to_fb(4_px));
        shapes[6].add_shape(view, {20_vp, 20_vp, 25_vp, 25_vp}, view.px_to_fb(5_px));

        for (VariantShape& shape : shapes) {
            if (&shape == &shapes[0] || &shape == &shapes[1]) {
                shape.update(Color(0, 0, 40, 128), Color(180, 180, 0), softness, antialiasing);
            } else {
                shape.update(Color(40, 40, 0, 128), Color(255, 255, 0), softness, antialiasing);
            }
        }
    };

    window.set_key_callback([&](View& view, KeyEvent ev) {
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
                for (auto& vs : shapes)
                    vs.switch_type<Rectangle>(renderer);
                break;
            case Key::O:
                for (auto& vs : shapes)
                    vs.switch_type<RoundedRectangle>(renderer);
                break;
            case Key::E:
                for (auto& vs : shapes)
                    vs.switch_type<Ellipse>(renderer);
                break;
            case Key::L:
                for (auto& vs : shapes)
                    vs.switch_type<Line>(renderer);
                break;
            case Key::P:
                for (auto& vs : shapes)
                    vs.switch_type<Polygon>(renderer);
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
