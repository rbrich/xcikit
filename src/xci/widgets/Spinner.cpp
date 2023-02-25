// Spinner.cpp created on 2023-02-25 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Spinner.h"

namespace xci::widgets {


Spinner::Spinner(Theme& theme, float value)
        : Widget(theme),
          m_bg_rect(theme.renderer()),
          m_arrow_color(theme.color(ColorId::Default)),
          m_outline_color(theme.color(ColorId::Default)),
          m_value(value)
{
    set_focusable(true);
    m_layout.set_default_font(&theme.base_font());
    update_text();
}


void Spinner::resize(View& view)
{
    view.finish_draw();
    m_layout.typeset(view);
    m_layout.update(view);
    auto rect = m_layout.bbox();
    apply_padding(rect, view);
    set_size(rect.size());
    set_baseline(-rect.y);
    Widget::resize(view);

    rect.x = 0;
    rect.y = 0;
    m_bg_rect.clear();
    m_bg_rect.add_rectangle(rect, view.to_fb(m_outline_thickness));
    m_bg_rect.update(m_fill_color, m_outline_color);
}


void Spinner::update(View& view, State state)
{
    Color focus;
    if (state.focused) {
        focus = theme().color(ColorId::Focus);
    } else if (last_hover() == LastHover::Inside) {
        focus = theme().color(ColorId::Hover);
    } else {
        focus = theme().color(ColorId::Default);
    }
    if (!m_outline_color.is_transparent())
        m_outline_color = focus;
    m_arrow_color = focus;
    m_bg_rect.update(m_fill_color, m_outline_color);
}


void Spinner::draw(View& view)
{
    const auto layout_pos = m_layout.bbox().top_left();
    const auto padding = padding_fb(view);
    m_bg_rect.draw(view, position());
    m_bg_rect.draw(view, position());
    m_layout.draw(view, position() + FramebufferCoords{padding - layout_pos.x, padding - layout_pos.y});
}


bool Spinner::key_event(View& view, const KeyEvent& ev)
{
    if (ev.action == Action::Press && ev.key == Key::Enter) {
        do_click(view);
        return true;
    }
    return false;
}


void Spinner::mouse_pos_event(View& view, const MousePosEvent& ev)
{
    do_hover(view, contains(ev.pos - view.offset()));
}


bool Spinner::mouse_button_event(View& view, const MouseBtnEvent& ev)
{
    if (ev.action == Action::Press &&
        ev.button == MouseButton::Left &&
        contains(ev.pos - view.offset()))
    {
        do_click(view);
        return true;
    }
    return false;
}


void Spinner::scroll_event(View& view, const ScrollEvent& ev)
{
    if (last_hover() != LastHover::Inside)
        return;

    m_value += m_step * round(ev.offset.y * 10);
    if (m_value > m_upper_bound)
        m_value = m_upper_bound;
    if (m_value < m_lower_bound)
        m_value = m_lower_bound;
    update_text();
    view.finish_draw();
    m_layout.typeset(view);
    m_layout.update(view);
    view.refresh();
}


void Spinner::update_text()
{
    m_layout.clear();
    m_layout.add_word(m_format_cb(m_value));
}


} // namespace xci::widgets
