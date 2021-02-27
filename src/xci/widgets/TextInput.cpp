// TextInput.cpp created on 2018-06-02, part of XCI toolkit
// Copyright 2018, 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
      m_bg_rect(theme.renderer(), Color(10, 20, 40), theme.color(ColorId::Default)),
      m_cursor_shape(theme.renderer(), Color::Yellow(), Color::Transparent())
{
    set_focusable(true);
    m_layout.set_default_font(&theme.font());
}


void TextInput::set_string(const std::string& string)
{
    m_buffer.set_content(string);
}


void TextInput::set_decoration_color(const graphics::Color& fill,
                                     const graphics::Color& outline)
{
    m_bg_rect.set_fill_color(fill);
    m_bg_rect.set_outline_color(outline);
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
    cursor_box.w = view.size_to_viewport(1_sc);
    if (cursor_box.x < m_content_pos)
        m_content_pos = cursor_box.x;
    if (cursor_box.x > m_content_pos + m_width)
        m_content_pos = cursor_box.x - m_width;
    m_cursor_shape.add_rectangle(cursor_box);
    m_cursor_shape.update();

    auto rect = m_layout.bbox();
    rect.w = m_width;
    rect.enlarge(m_padding);
    set_size(rect.size());
    set_baseline(-rect.y);

    // Background rect
    rect.x = 0;
    rect.y = 0;
    m_bg_rect.clear();
    m_bg_rect.add_rectangle(rect, m_outline_thickness);
    m_bg_rect.update();
}


void TextInput::update(View& view, State state)
{
    view.finish_draw();
    m_layout.update(view);
    if (last_hover() == LastHover::Inside) {
        m_bg_rect.set_outline_color(theme().color(ColorId::Hover));
    } else {
        m_bg_rect.set_outline_color(theme().color(ColorId::Default));
    }
    m_bg_rect.update();
    m_draw_cursor = state.focused;
    if (m_draw_cursor)
        m_cursor_shape.update();
}


void TextInput::draw(View& view)
{
    auto rect = m_layout.bbox();
    auto pos = position() + ViewportCoords{m_padding - rect.x - m_content_pos,
                                           m_padding - rect.y};
    m_bg_rect.draw(view, position());
    view.push_crop(aabb().enlarged(-m_outline_thickness));
    m_layout.draw(view, pos);
    if (m_draw_cursor)
        m_cursor_shape.draw(view, pos);
    view.pop_crop();
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
            if (!m_buffer.move_to_home())
                return true;
            break;

        case Key::End:
            if (!m_buffer.move_to_end())
                return true;
            break;

        default:
            return false;
    }

    resize(view);
    view.refresh();
    if (m_change_cb)
        m_change_cb(view);
    return true;
}


void TextInput::char_event(View& view, const CharEvent& ev)
{
    auto ch = to_utf8(ev.code_point);
    m_buffer.insert(ch);
    resize(view);
    view.refresh();
    if (m_change_cb)
        m_change_cb(view);
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
