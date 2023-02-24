// ColorPicker.h created on 2023-02-24 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_WIDGETS_COLOR_PICKER_H
#define XCI_WIDGETS_COLOR_PICKER_H

#include <xci/widgets/Widget.h>
#include <xci/text/Layout.h>

namespace xci::widgets {


class ColorPicker: public Widget, public Clickable, public Padded {
public:
    explicit ColorPicker(Theme& theme, Color color);

    void set_color(Color color) { m_color = color; }
    Color color() const { return m_color; }

    using ChangeCallback = std::function<void(View&)>;
    void on_change(ChangeCallback cb) { m_change_cb = std::move(cb); }

    void resize(View& view) override;
    void update(View& view, State state) override;
    void draw(View& view) override;
    bool key_event(View& view, const KeyEvent& ev) override;
    void mouse_pos_event(View& view, const MousePosEvent& ev) override;
    bool mouse_button_event(View& view, const MouseBtnEvent& ev) override;

private:
    void update_text();

    text::Layout m_layout;
    graphics::Rectangle m_sample_box;  // a sample of the color
    Color m_color;  // the color being edited
    Color m_decoration;  // outline and text color
    ChangeCallback m_change_cb;
};


} // namespace xci::widgets

#endif  // include guard
