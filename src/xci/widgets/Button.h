// Button.h created on 2018-03-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_WIDGETS_BUTTON_H
#define XCI_WIDGETS_BUTTON_H

#include <xci/widgets/Widget.h>
#include <xci/graphics/Shape.h>
#include <xci/graphics/Color.h>
#include <xci/text/Font.h>
#include <xci/text/Text.h>

namespace xci::widgets {


class Button: public Widget, public Clickable {
public:
    explicit Button(Theme& theme, const std::string &string);

    void set_font_size(ViewportUnits size) { m_layout.set_default_font_size(size); }
    void set_padding(ViewportUnits padding) { m_padding = padding; }
    void set_outline_thickness(ViewportUnits thickness) { m_outline_thickness = thickness; }

    void set_decoration_color(graphics::Color fill, graphics::Color outline);
    void set_text_color(graphics::Color color);

    void resize(View& view) override;
    void update(View& view, State state) override;
    void draw(View& view) override;
    bool key_event(View& view, const KeyEvent& ev) override;
    void mouse_pos_event(View& view, const MousePosEvent& ev) override;
    bool mouse_button_event(View& view, const MouseBtnEvent& ev) override;

private:
    graphics::Shape m_bg_rect;
    text::Layout m_layout;
    ViewportUnits m_padding = 0.02f;
    ViewportUnits m_outline_thickness = 0.005f;
};


} // namespace xci::widgets

#endif // XCI_WIDGETS_BUTTON_H
