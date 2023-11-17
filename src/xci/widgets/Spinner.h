// Spinner.h created on 2023-02-25 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_WIDGETS_SPINNER_H
#define XCI_WIDGETS_SPINNER_H

#include <xci/widgets/Widget.h>
#include <xci/text/Layout.h>
#include <xci/graphics/shape/Triangle.h>
#include <functional>

namespace xci::widgets {


class Spinner: public Widget, public Clickable, public Padded {
public:
    explicit Spinner(Theme& theme, float value);

    void set_value(float value) { m_value = value; }
    float value() const { return m_value; }

    void set_step(float step, float big_step) { m_step = step; m_big_step = big_step; }
    void set_bounds(float lower, float upper) { m_lower_bound = lower; m_upper_bound = upper; }

    void set_outline_thickness(VariUnits thickness) { m_outline_thickness = thickness; }
    void set_decoration_color(graphics::Color fill, graphics::Color outline);
    void set_text_color(graphics::Color color);

    using FormatCb = std::function<std::string(float value)>;
    static std::string default_format_cb(float v) { return fmt::format("{:.2f}", v); }
    void set_format_cb(FormatCb&& cb) { m_format_cb = std::move(cb); update_text(); }

    using ChangeCallback = std::function<void(Spinner&)>;
    void on_change(ChangeCallback cb) { m_change_cb = std::move(cb); }

    void resize(View& view) override;
    void update(View& view, State state) override;
    void draw(View& view) override;
    bool key_event(View& view, const KeyEvent& ev) override;
    void mouse_pos_event(View& view, const MousePosEvent& ev) override;
    bool mouse_button_event(View& view, const MouseBtnEvent& ev) override;
    void scroll_event(View& view, const ScrollEvent& ev) override;

private:
    void update_text();
    void update_arrows(View& view);
    void change_value(View& view, float change);  // check bounds, call change_cb

    text::Layout m_layout;
    graphics::Rectangle m_bg_rect;
    graphics::ColoredTriangle m_arrow;
    Color m_fill_color = Color(10, 20, 40);
    Color m_arrow_color;
    Color m_outline_color;
    VariUnits m_outline_thickness = 0.25_vp;
    ChangeCallback m_change_cb;

    FormatCb m_format_cb = default_format_cb;
    float m_value = 0.0f;
    float m_step = 0.01f;
    float m_big_step = 0.10f;
    float m_lower_bound = 0.0f;
    float m_upper_bound = 1.0f;
};


} // namespace xci::widgets

#endif  // include guard
