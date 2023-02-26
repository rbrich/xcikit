// TextInput.cpp created on 2018-06-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "TextInput.h"
#include <xci/core/string.h>
#include <xci/core/geometry.h>

namespace xci::widgets {

using namespace xci::graphics;
using namespace xci::text;
using namespace xci::core;


TextInput::TextInput(Theme& theme, const std::string& string)
    : Widget(theme),
      m_buffer(string),
      m_bg_rect(theme.renderer()),
      m_cursor_shape(theme.renderer()),
      m_outline_color(theme.color(ColorId::Default))
{
    set_focusable(true);
    m_layout.set_default_font(&theme.base_font());
}


void TextInput::set_string(const std::string& string)
{
    m_buffer.set_content(string);
}


void TextInput::set_decoration_color(graphics::Color fill,
                                     graphics::Color outline)
{
    m_fill_color = fill;
    m_outline_color = outline;
}


void TextInput::resize(View& view)
{
    view.finish_draw();
    m_layout.clear();
    // Text before cursor
    m_layout.add_word(m_buffer.content_upto_cursor());
    // Cursor placing
    m_layout.begin_span("cursor");
    m_layout.add_word("");
    m_layout.end_span("cursor");
    // Text after cursor
    m_layout.add_word(m_buffer.content_from_cursor());
    m_layout.typeset(view);
    m_layout.update(view);

    // Cursor rect
    layout::Span* cursor_span = m_layout.get_span("cursor");
    m_cursor_shape.clear();
    auto cursor_box = cursor_span->part(0).bbox();
    cursor_box.w = view.px_to_fb(1_px);
    if (cursor_box.x < m_content_pos)
        m_content_pos = cursor_box.x;
    const auto width = view.to_fb(m_width);
    if (cursor_box.x > m_content_pos + width)
        m_content_pos = cursor_box.x - width;
    m_cursor_shape.add_rectangle(cursor_box);
    m_cursor_shape.update(Color::Yellow());

    auto rect = m_layout.bbox();
    rect.w = width;
    apply_padding(rect, view);
    set_size(rect.size());
    set_baseline(-rect.y);
    Widget::resize(view);

    // Background rect
    rect.x = 0;
    rect.y = 0;
    m_bg_rect.clear();
    m_bg_rect.add_rectangle(rect, view.to_fb(m_outline_thickness));
    m_bg_rect.update(m_fill_color, m_outline_color);
}


void TextInput::update(View& view, State state)
{
    view.finish_draw();
    m_layout.update(view);
    if (state.focused) {
        m_outline_color = theme().color(ColorId::Focus);
    } else if (last_hover() == LastHover::Inside) {
        m_outline_color = theme().color(ColorId::Hover);
    } else {
        m_outline_color = theme().color(ColorId::Default);
    }
    m_bg_rect.update(m_fill_color, m_outline_color);
    m_draw_cursor = state.focused;
    if (m_draw_cursor)
        m_cursor_shape.update(Color::Yellow());
}


void TextInput::draw(View& view)
{
    auto rect = m_layout.bbox();
    const auto padding = padding_fb(view);
    auto pos = position() + FramebufferCoords{padding.x - rect.x - m_content_pos,
                                              padding.y - rect.y};
    m_bg_rect.draw(view, position());
    auto pop_crop = view.push_crop(aabb().enlarged(-view.to_fb(m_outline_thickness)));
    m_layout.draw(view, pos);
    if (m_draw_cursor)
        m_cursor_shape.draw(view, pos);
}


bool TextInput::key_event(View& view, const KeyEvent& ev)
{
    // Ignore key release, handle only press and repeat
    if (ev.action == Action::Release)
        return false;

    switch (ev.key) {
        case Key::Backspace:
            if (!m_buffer.delete_left())
                return true;
            break;

        case Key::Delete:
            if (!m_buffer.delete_right())
                return true;
            break;

        case Key::Left:
            if (!m_buffer.move_left())
                return true;
            break;

        case Key::Right:
            if (!m_buffer.move_right())
                return true;
            break;

        case Key::Home:
            if (!m_buffer.move_to_line_beginning())
                return true;
            break;

        case Key::End:
            if (!m_buffer.move_to_line_end())
                return true;
            break;

        default:
            return false;
    }

    resize(view);
    view.refresh();
    if (m_change_cb)
        m_change_cb(*this);
    return true;
}


void TextInput::char_event(View& view, const CharEvent& ev)
{
    auto ch = to_utf8(ev.code_point);
    m_buffer.insert(ch);
    resize(view);
    view.refresh();
    if (m_change_cb)
        m_change_cb(*this);
}


void TextInput::mouse_pos_event(View& view, const MousePosEvent& ev)
{
    do_hover(view, contains(ev.pos - view.offset()));
}


bool TextInput::mouse_button_event(View& view, const MouseBtnEvent& ev)
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
