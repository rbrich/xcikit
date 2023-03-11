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
          m_arrow(theme.renderer()),
          m_arrow_color(theme.color(ColorId::Default)),
          m_outline_color(theme.color(ColorId::Default)),
          m_value(value)
{
    set_focusable(true);
    m_arrow.reserve(2);
    m_layout.set_default_font(&theme.base_font());
    update_text();
}


void Spinner::set_decoration_color(graphics::Color fill, graphics::Color outline)
{
    m_fill_color = fill;
    m_outline_color = outline;
}


void Spinner::set_text_color(graphics::Color color)
{
    m_layout.set_default_color(color);
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

    update_arrows(view);
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
    update_arrows(view);
}


void Spinner::draw(View& view)
{
    const auto layout_pos = m_layout.bbox().top_left();
    const auto padding = padding_fb(view);
    m_bg_rect.draw(view, position());
    m_bg_rect.draw(view, position());
    m_layout.draw(view, position() + FramebufferCoords{padding.x - layout_pos.x,
                                                       padding.y - layout_pos.y});
    m_arrow.draw(view, position());
}


bool Spinner::key_event(View& view, const KeyEvent& ev)
{
    if (ev.action == Action::Release)
        return false;

    switch (ev.key) {
        case Key::Enter:
            do_click(view);
            return true;

        case Key::Up:
            change_value(view, m_step);
            return true;
        case Key::Down:
            change_value(view, -m_step);
            return true;
        case Key::PageUp:
            change_value(view, m_big_step);
            return true;
        case Key::PageDown:
            change_value(view, -m_big_step);
            return true;

        default:
            return false;
    }
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

    change_value(view, m_step * std::round(ev.offset.y * 10));
}


void Spinner::update_text()
{
    m_layout.clear();
    m_layout.add_word(m_format_cb(m_value));
}


void Spinner::update_arrows(View& view)
{
    const auto sz = size();
    const auto th = view.to_fb(m_outline_thickness);
    const auto mx = sz.x / 2;
    const auto h = sz.y;
    const auto p = padding_fb(view).y;
    m_arrow.clear();
    m_arrow.add_triangle({mx, th + 0.2f * p},
                         {mx - p, 1.3f * p},
                         {mx + p, 1.3f * p},
                         m_arrow_color);
    m_arrow.add_triangle({mx, h - (th + 0.2f * p)},
                         {mx + p, h - 1.3f * p},
                         {mx - p, h - 1.3f * p},
                         m_arrow_color);
    m_arrow.update(0, 1);
}


void Spinner::change_value(View& view, float change)
{
    m_value += change;
    if (m_value > m_upper_bound)
        m_value = m_upper_bound;
    if (m_value < m_lower_bound)
        m_value = m_lower_bound;
    if (m_change_cb)
        m_change_cb(*this);
    update_text();
    view.finish_draw();
    m_layout.typeset(view);
    m_layout.update(view);
    view.refresh();
}


} // namespace xci::widgets
