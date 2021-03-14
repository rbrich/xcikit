// Checkbox.h created on 2018-04-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_WIDGETS_CHECKBOX_H
#define XCI_WIDGETS_CHECKBOX_H

#include <xci/widgets/Icon.h>
#include <xci/graphics/View.h>
#include <xci/graphics/Window.h>

namespace xci::widgets {

using widgets::Theme;
using graphics::View;
using graphics::MouseBtnEvent;


// FIXME: Checkbox has-an Icon -> m_icon
class Checkbox: public Icon, public Clickable {
public:
    explicit Checkbox(Theme& theme);

    void set_checked(bool checked);
    bool checked() const { return m_checked; }

    using ChangeCallback = std::function<void()>;
    void on_change(ChangeCallback cb) { m_change_cb = std::move(cb); }

    bool key_event(View& view, const KeyEvent& ev) override;
    void mouse_pos_event(View& view, const MousePosEvent& ev) override;
    bool mouse_button_event(View& view, const MouseBtnEvent& ev) override;

private:
    bool m_checked = false;
    ChangeCallback m_change_cb;
};


} // namespace xci::widgets

#endif // XCI_WIDGETS_CHECKBOX_H
