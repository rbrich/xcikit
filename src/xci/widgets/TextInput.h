// TextInput.h created on 2018-06-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_WIDGETS_TEXTINPUT_H
#define XCI_WIDGETS_TEXTINPUT_H

#include <xci/widgets/Widget.h>
#include <xci/text/Layout.h>
#include <xci/graphics/Shape.h>
#include <xci/core/EditBuffer.h>

namespace xci::widgets {


class TextInput: public Widget, public Clickable {
public:
    explicit TextInput(Theme& theme, const std::string& string);

    void set_string(const std::string& string);
    const std::string& string() const { return m_buffer.content(); }

    void set_font_size(ViewportUnits size) { m_layout.set_default_font_size(size); }
    void set_width(ViewportUnits width) { m_width = width; }
    void set_padding(ViewportUnits padding) { m_padding = padding; }
    void set_outline_thickness(ViewportUnits thickness) { m_outline_thickness = thickness; }

    void set_decoration_color(graphics::Color fill, graphics::Color outline);
    void set_text_color(graphics::Color color) { m_layout.set_default_color(color); }

    using ChangeCallback = std::function<void(View&)>;
    void on_change(ChangeCallback cb) { m_change_cb = std::move(cb); }

    void resize(View& view) override;
    void update(View& view, State state) override;
    void draw(View& view) override;
    bool key_event(View& view, const KeyEvent& ev) override;
    void char_event(View& view, const CharEvent& ev) override;
    void mouse_pos_event(View& view, const MousePosEvent& ev) override;
    bool mouse_button_event(View& view, const MouseBtnEvent& ev) override;

private:
    core::EditBuffer m_buffer;
    text::Layout m_layout;
    graphics::Shape m_bg_rect;
    graphics::Shape m_cursor_shape;
    ViewportUnits m_width = 0.4f;
    ViewportUnits m_padding = 0.02f;
    ViewportUnits m_content_pos = 0;
    ViewportUnits m_outline_thickness = 0.005f;
    ChangeCallback m_change_cb;
    bool m_draw_cursor = false;
};


} // namespace xci::widgets

#endif // XCI_WIDGETS_TEXTINPUT_H
