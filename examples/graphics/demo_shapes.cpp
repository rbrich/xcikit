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
#include <xci/graphics/shape/Triangle.h>
#include <xci/vfs/Vfs.h>
#include <xci/config.h>
#include <variant>
#include <cstdlib>

using namespace xci::text;


class VariantShape {
public:
    VariantShape(Renderer& renderer) : m_shape(std::in_place_type<ColoredRectangle>, renderer) {}

    template <typename T>
    void switch_type(Renderer& renderer) {
        m_shape.emplace<T>(renderer);
    }

    void add_shape(View& view, const ViewportRect& vp_rect,
                   Color fill_color, Color outline_color, FramebufferPixels th)
    {
        std::visit([&view, rect = view.vp_to_fb(vp_rect), fill_color, outline_color, th] (auto& shape) {
            using T = std::decay_t<decltype(shape)>;
            if constexpr (std::is_same_v<T, ColoredRectangle>) {
                shape.add_rectangle(rect, fill_color, outline_color, th);
            } else if constexpr (std::is_same_v<T, ColoredRoundedRectangle>) {
                shape.add_rounded_rectangle(rect, view.vp_to_fb(2.5_vp), fill_color, outline_color, th);
            } else if constexpr (std::is_same_v<T, ColoredEllipse>) {
                shape.add_ellipse(rect, fill_color, outline_color, th);
            } else if constexpr (std::is_same_v<T, ColoredLine>) {
                const auto l = rect.left();
                const auto t = rect.top();
                const auto r = rect.right();
                const auto b = rect.bottom();
                const auto w2 = rect.w / 2;
                const auto h2 = rect.h / 2;
                const auto w4 = rect.w / 4;
                const auto h4 = rect.h / 4;
                const auto c = rect.center();
                shape.add_line_slice({l, t, w2, h2}, {l, t+h4}, {c.x, t}, fill_color, outline_color, th);
                shape.add_line_slice({c.x, t, w2, h2}, {r-w4, t}, {r, c.y}, fill_color, outline_color, th);
                shape.add_line_slice({c.x, c.y, w2, h2}, {r, b-h4}, {c.x, b}, fill_color, outline_color, th);
                shape.add_line_slice({l, c.y, w2, h2}, {l+w4, b}, {l, c.y}, fill_color, outline_color, th);
            } else if constexpr (std::is_same_v<T, ColoredPolygon>) {
                constexpr int edges = 14;
                const float angle = 2 * std::acos(-1) / edges;
                const auto center = rect.center();
                std::vector<FramebufferCoords> vertices;
                for (int i = 0; i <= edges; i++) {
                    vertices.emplace_back(center + ((0.3 + 0.2 * (i % 2)) * rect.size()).rotate(-angle * i));
                }
                shape.add_polygon(center, vertices, fill_color, outline_color, th);
            } else if constexpr (std::is_same_v<T, ColoredTriangle>) {
                shape.add_triangle({rect.x, rect.y}, {rect.x, rect.y+rect.h}, {rect.x+rect.w, rect.y},
                                   fill_color, outline_color, th);
            }
        }, m_shape);
    }

    void clear() {
        std::visit([](auto& shape) {
            shape.clear();
        }, m_shape);
    }

    void update(float softness, float antialiasing) {
        std::visit([=](auto& shape) {
            shape.update(softness, antialiasing);
        }, m_shape);
    }

    void draw(View& view, VariCoords pos) {
        std::visit([&view, pos](auto& shape) {
            shape.draw(view, pos);
        }, m_shape);
    }

private:
    std::variant<ColoredRectangle, ColoredRoundedRectangle, ColoredEllipse, ColoredLine,
                 ColoredPolygon, ColoredTriangle> m_shape;
};


int main(int argc, char* argv[])
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
                           "[p] polygons\n"
                           "[t] triangles\n");
    shapes_help.set_color(Color(200, 100, 50));
    Text option_help(font, "[a] antialiasing\n"
                           "[s] softness\n");
    option_help.set_color(Color(200, 100, 50));

    VariantShape shape { renderer };
    bool antialiasing = false;
    bool softness = false;

    auto recreate_shapes = [&](View& view) {
        view.finish_draw();

        shape.clear();

        // Border scaled with viewport size
        shape.add_shape(view, {-50_vp, -30_vp, 100_vp, 60_vp}, Color(0, 0, 40, 128), Color(180, 180, 0), view.vp_to_fb(2.5_vp));
        shape.add_shape(view, {-30_vp, -40_vp, 60_vp, 80_vp}, Color(0, 0, 40, 128), Color(180, 180, 0), view.vp_to_fb(1_vp));

        // Constant border width, in screen pixels
        for (size_t i = 0; i < 5; i++)
            shape.add_shape(view, {-22.5_vp + i * 5_vp, -22.5_vp + i * 5_vp, 25_vp, 25_vp},
                            Color(40, 40, 0, 128), Color(255, 255, 0), view.px_to_fb((i + 1) * 1_px));

        shape.update(int(softness) * 1.0f, int(antialiasing) * 2.0f);
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
                shape.switch_type<ColoredRectangle>(renderer);
                break;
            case Key::O:
                shape.switch_type<ColoredRoundedRectangle>(renderer);
                break;
            case Key::E:
                shape.switch_type<ColoredEllipse>(renderer);
                break;
            case Key::L:
                shape.switch_type<ColoredLine>(renderer);
                break;
            case Key::P:
                shape.switch_type<ColoredPolygon>(renderer);
                break;
            case Key::T:
                shape.switch_type<ColoredTriangle>(renderer);
                break;
            case Key::A:
                antialiasing = !antialiasing;
                break;
            case Key::S:
                softness = !softness;
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

        shape.draw(view, {0_vp, 0_vp});
    });

    window.set_refresh_mode(RefreshMode::OnDemand);
    window.display();
    return EXIT_SUCCESS;
}
