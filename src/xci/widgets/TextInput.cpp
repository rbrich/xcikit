// TextInput.cpp created on 2018-06-02, part of XCI toolkit
// Copyright 2018 Radek Brich
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
#include <xci/util/string.h>
#include <xci/util/geometry.h>

namespace xci {
namespace widgets {

using namespace xci::graphics;
using namespace xci::text;
using namespace xci::util;


TextInput::TextInput(const std::string& string)
    : m_text(string),
      m_cursor(string.size()),
      m_bg_rect(Color(10, 20, 40), Color(180, 180, 180)),
      m_cursor_shape(Color::Yellow(), Color::Transparent())
{
    m_layout.set_default_font(&theme().font());
}


void TextInput::set_decoration_color(const graphics::Color& fill,
                                     const graphics::Color& border)
{
    m_bg_rect = Shape(fill, border);
}


void TextInput::resize(View& view)
{
    m_layout.clear();
    // Text before cursor
    m_layout.add_word(m_text.substr(0, m_cursor));
    // Cursor placing
    m_layout.begin_span("cursor");
    m_layout.add_word("");
    m_layout.end_span("cursor");
    // Text after cursor
    m_layout.add_word(m_text.substr(m_cursor));
    m_layout.typeset(view);

    // Cursor rect
    layout::Span* cursor_span = m_layout.get_span("cursor");
    m_cursor_shape.clear();
    auto cursor_box = cursor_span->part(0).bbox();
    cursor_box.w = view.screen_ratio().x;
    if (cursor_box.x < m_content_pos)
        m_content_pos = cursor_box.x;
    if (cursor_box.x > m_content_pos + m_width)
        m_content_pos = cursor_box.x - m_width;
    m_cursor_shape.add_rectangle(cursor_box);

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
}


void TextInput::draw(View& view, State state)
{
    auto rect = m_layout.bbox();
    auto pos = position() + Vec2f{m_padding - rect.x - m_content_pos,
                                  m_padding - rect.y};
    m_bg_rect.draw(view, position());
    view.push_crop(aabb().enlarged(-m_outline_thickness));
    m_layout.draw(view, pos);
    if (state.focused)
        m_cursor_shape.draw(view, pos);
    view.pop_crop();
}


void TextInput::handle(View& view, const KeyEvent& ev)
{
    // Ignore key release, handle only press and repeat
    if (ev.action == Action::Release)
        return;

    switch (ev.key) {
        case Key::Backspace:
            if (m_cursor > 0) {
                auto pos = m_text.rbegin() + m_text.size() - m_cursor;
                auto prev = m_text.rbegin() + m_text.size() - utf8_prev(pos);
                m_text.erase(prev, m_cursor - prev);
                m_cursor = prev;
                break;
            }
            return;

        case Key::Delete:
            if (m_cursor < m_text.size()) {
                auto next = utf8_next(m_text.begin() + m_cursor) - m_text.begin();
                m_text.erase(m_cursor, next - m_cursor);
                break;
            }
            return;

        case Key::Left:
            if (m_cursor > 0) {
                auto pos = m_text.rbegin() + m_text.size() - m_cursor;
                m_cursor = m_text.rbegin() + m_text.size() - utf8_prev(pos);
                break;
            }
            return;

        case Key::Right:
            if (m_cursor < m_text.size()) {
                m_cursor = utf8_next(m_text.begin() + m_cursor) - m_text.begin();
                break;
            }
            return;

        case Key::Home:
            if (m_cursor > 0) {
                m_cursor = 0;
                break;
            }
            return;

        case Key::End:
            if (m_cursor < m_text.size()) {
                m_cursor = m_text.size();
                break;
            }
            return;

        default:
            return;
    }

    resize(view);
    view.refresh();
}


void TextInput::handle(View& view, const CharEvent& ev)
{
    auto ch = to_utf8(ev.code_point);
    m_text.insert(m_cursor, ch);
    m_cursor += ch.size();
    resize(view);
    view.refresh();
}


void TextInput::handle(View& view, const MouseBtnEvent& ev)
{
    Widget::handle(view, ev);
}


}} // namespace xci::widgets
