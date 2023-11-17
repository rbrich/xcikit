// TextInput.h created on 2018-06-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_WIDGETS_TEXTINPUT_H
#define XCI_WIDGETS_TEXTINPUT_H

#include <xci/widgets/Widget.h>
#include <xci/text/Layout.h>
#include <xci/graphics/shape/Rectangle.h>
#include <xci/core/EditBuffer.h>

namespace xci::widgets {

using xci::graphics::VariUnits;
using xci::graphics::FramebufferPixels;
using namespace xci::graphics::unit_literals;


class TextInput: public Widget, public Clickable, public Padded {
public:
    explicit TextInput(Theme& theme, const std::string& string);

    void set_string(const std::string& string);
    const std::string& string() const { return m_buffer.content(); }

    void set_font_size(VariUnits size) { m_layout.set_default_font_size(size); }
    void set_width(VariUnits width) { m_width = width; }
    void set_outline_thickness(VariUnits thickness) { m_outline_thickness = thickness; }

    void set_decoration_color(graphics::Color fill, graphics::Color outline);
    void set_text_color(graphics::Color color) { m_layout.set_default_color(color); }

    using ChangeCallback = std::function<void(TextInput&)>;
    void on_change(ChangeCallback cb) { m_change_cb = std::move(cb); }

    void resize(View& view) override;
    void update(View& view, State state) override;
    void draw(View& view) override;
    bool key_event(View& view, const KeyEvent& ev) override;
    void text_input_event(View& view, const TextInputEvent& ev) override;
    void mouse_pos_event(View& view, const MousePosEvent& ev) override;
    bool mouse_button_event(View& view, const MouseBtnEvent& ev) override;
    void focus_change(View& view, const FocusChange& ev) override;

private:
    core::EditBuffer m_buffer;
    core::EditBuffer m_ime_buffer;
    text::Layout m_layout;
    graphics::Rectangle m_bg_rect;
    graphics::Rectangle m_cursor_shape;
    VariUnits m_width = 20_vp;
    VariUnits m_outline_thickness = 0.25_vp;
    Color m_fill_color = Color(10, 20, 40);
    Color m_outline_color;
    FramebufferPixels m_content_pos = 0;
    ChangeCallback m_change_cb;
    bool m_draw_cursor = false;
};


} // namespace xci::widgets

#endif // XCI_WIDGETS_TEXTINPUT_H
