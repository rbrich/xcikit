// Button.cpp created on 2018-03-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Button.h"
#include <xci/text/Markup.h>
#include <xci/graphics/Renderer.h>

namespace xci::widgets {

using namespace xci::graphics;
using namespace xci::text;


Button::Button(Theme& theme, const std::string &string)
    : Widget(theme),
      m_bg_rect(theme.renderer()),
      m_outline_color(theme.color(ColorId::Default))
{
    set_focusable(true);
    m_layout.set_default_font(&theme.base_font());
    Markup markup(m_layout);
    markup.parse(string);
}


void Button::set_decoration_color(graphics::Color fill,
                                  graphics::Color outline)
{
    m_fill_color = fill;
    m_outline_color = outline;
}


void Button::set_text_color(graphics::Color color)
{
    m_layout.set_default_color(color);
}


void Button::resize(View& view)
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


void Button::update(View& view, State state)
{
    if (state.focused) {
        m_outline_color = theme().color(ColorId::Focus);
    } else if (last_hover() == LastHover::Inside) {
        m_outline_color = theme().color(ColorId::Hover);
    } else {
        m_outline_color = theme().color(ColorId::Default);
    }
    m_bg_rect.update(m_fill_color, m_outline_color);
}


void Button::draw(View& view)
{
    const auto layout_pos = m_layout.bbox().top_left();
    const auto padding = padding_fb(view);
    m_bg_rect.draw(view, position());
    m_layout.draw(view, position() + FramebufferCoords{padding - layout_pos.x, padding - layout_pos.y});
}


bool Button::key_event(View& view, const KeyEvent& ev)
{
    if (ev.action == Action::Press && ev.key == Key::Enter) {
        do_click(view);
        return true;
    }
    return false;
}


void Button::mouse_pos_event(View& view, const MousePosEvent& ev)
{
    do_hover(view, contains(ev.pos - view.offset()));
}


bool Button::mouse_button_event(View& view, const MouseBtnEvent& ev)
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


} // namespace xci::widgets
