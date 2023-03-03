// CoordEditor.h created on 2023-03-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SHED_COORD_EDITOR_H
#define XCI_SHED_COORD_EDITOR_H

#include "ShaderCompiler.h"

#include <xci/widgets/Widget.h>
#include <xci/graphics/Primitives.h>
#include <xci/graphics/shape/Polygon.h>
#include <xci/graphics/shape/Triangle.h>
#include <xci/graphics/shape/Ellipse.h>

#include <vector>
#include <functional>

namespace xci::shed {

using namespace xci::widgets;
using namespace xci::graphics;


class CoordEditor: public Widget {
public:
    explicit CoordEditor(Theme& theme, Primitives& prim);

    void toggle_triangle() { m_is_triangle = !m_is_triangle; }

    using ChangeCallback = std::function<void(CoordEditor&)>;
    void on_change(ChangeCallback cb) { m_change_cb = std::move(cb); }

    void resize(View& view) override;
    void update(View& view, State state) override;
    void draw(View& view) override;

    void mouse_pos_event(View& view, const MousePosEvent& ev) override;
    bool mouse_button_event(View& view, const MouseBtnEvent& ev) override;

private:
    void reconstruct(View& view);

    Primitives& m_prim;
    Polygon m_poly;
    Triangle m_triangle;
    ColoredEllipse m_circles;
    ChangeCallback m_change_cb;

    struct Point {
        ViewportCoords xy;
        Vec2f uv;
    };
    std::vector<Point> m_triangle_vertices {
            {{-49_vp, -49_vp}, {0.0, 0.0}},
            {{-49_vp, +49_vp}, {0.5, 0.0}},
            {{+49_vp, +49_vp}, {1.0, 1.0}},
    };
    std::vector<Point> m_quad_vertices {
            {{-49_vp, -49_vp}, {-1, -1}},
            {{-49_vp, +49_vp}, {-1, +1}},
            {{+49_vp, +49_vp}, {+1, +1}},
            {{+49_vp, -49_vp}, {+1, -1}},
    };
    unsigned m_active_vertex = ~0u;
    bool m_dragging = false;
    bool m_is_triangle = true;
    bool m_need_reconstruct = false;
};


}  // namespace xci::shed

#endif  // include guard
