// ColorPicker.cpp created on 2023-02-24 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ColorPicker.h"

namespace xci::widgets {


ColorPicker::ColorPicker(Theme& theme, Color color)
        : Widget(theme),
          m_sample_box(theme.renderer()),
          m_color(color),
          m_decoration(theme.color(ColorId::Default))
{
    set_focusable(true);
    m_layout.set_default_font(&theme.base_font());
    update_text();
}


void ColorPicker::resize(View& view)
{
    view.finish_draw();
    m_layout.typeset(view);
    m_layout.update(view);
    auto rect = m_layout.bbox();
    apply_padding(rect, view);

    rect.w += rect.h + padding_fb(view);  // make space for sample box
    set_size(rect.size());
    set_baseline(-rect.y);
    Widget::resize(view);

    rect.x = 0;
    rect.y = 0;
    rect.w = rect.h;
    m_sample_box.clear();
    m_sample_box.add_rectangle(rect, view.to_fb(1_px));
    m_sample_box.update(m_color, m_decoration);
}


void ColorPicker::update(View& view, State state)
{
    if (state.focused) {
        m_decoration = theme().color(ColorId::Focus);
    } else if (last_hover() == LastHover::Inside) {
        m_decoration = theme().color(ColorId::Hover);
    } else {
        m_decoration = theme().color(ColorId::Default);
    }
    m_sample_box.update(m_color, m_decoration);
}


void ColorPicker::draw(View& view)
{
    const auto layout_pos = m_layout.bbox().top_left();
    const auto padding = padding_fb(view);
    m_sample_box.draw(view, position());
    m_layout.draw(view, position()
                        + FramebufferCoords{padding - layout_pos.x + size().y + padding,
                                            padding - layout_pos.y}); // ^__ height+padding = sample box
}


bool ColorPicker::key_event(View& view, const KeyEvent& ev)
{
    if (ev.action == Action::Press && ev.key == Key::Enter) {
        do_click(view);
        return true;
    }
    return false;
}


void ColorPicker::mouse_pos_event(View& view, const MousePosEvent& ev)
{
    const auto p = ev.pos - view.offset();
    do_hover(view, contains(p));

    auto* r = m_layout.get_span("R");
    if (r && r->contains(p)) {
        r->adjust_color(Color::Red());
    }
}


bool ColorPicker::mouse_button_event(View& view, const MouseBtnEvent& ev)
{
    if (ev.action == Action::Press &&
        ev.button == MouseButton::Left)
    {
        const auto p = ev.pos - view.offset();
        if (contains(p)) {
            do_click(view);

            if (m_layout.get_span("R")->contains(p)) {

            }

            return true;
        }
    }
    return false;
}


void ColorPicker::update_text()
{
    m_layout.set_color(Color(192, 50, 0));
    m_layout.begin_span("R");
    m_layout.add_word(fmt::format("{:02X}", m_color.r));
    m_layout.end_span("R");
    m_layout.add_space();
    m_layout.set_color(Color::Green());
    m_layout.begin_span("G");
    m_layout.add_word(fmt::format("{:02X}", m_color.g));
    m_layout.end_span("G");
    m_layout.add_space();
    m_layout.set_color(Color(0, 100, 255));
    m_layout.begin_span("B");
    m_layout.add_word(fmt::format("{:02X}", m_color.b));
    m_layout.end_span("B");
    m_layout.add_space();
    m_layout.set_color(Color::Grey());
    m_layout.begin_span("A");
    m_layout.add_word(fmt::format("{:02X}", m_color.a));
    m_layout.end_span("A");
}


} // namespace xci::widgets
