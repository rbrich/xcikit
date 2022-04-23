// Button.h created on 2018-03-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_WIDGETS_BUTTON_H
#define XCI_WIDGETS_BUTTON_H

#include <xci/widgets/Widget.h>
#include <xci/graphics/Shape.h>
#include <xci/graphics/Color.h>
#include <xci/text/Font.h>
#include <xci/text/Text.h>

namespace xci::widgets {

using namespace xci::graphics::unit_literals;


class Button: public Widget, public Clickable {
public:
    explicit Button(Theme& theme, const std::string &string);

    void set_font_size(VariUnits size) { m_layout.set_default_font_size(size); }
    void set_padding(VariUnits padding) { m_padding = padding; }
    void set_outline_thickness(VariUnits thickness) { m_outline_thickness = thickness; }

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
    VariUnits m_padding = 1_vp;
    VariUnits m_outline_thickness = 0.25_vp;
};


} // namespace xci::widgets

#endif // XCI_WIDGETS_BUTTON_H
