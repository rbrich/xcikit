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
      m_bg_rect(Color(10, 20, 40), Color(180, 180, 0)),
      m_cursor_shape(Color(255, 255, 0), Color::Transparent())
{
    m_layout.set_default_font(&theme().font());
}


void TextInput::set_decoration_color(const graphics::Color& fill,
                                     const graphics::Color& border)
{
    m_bg_rect = Shape(fill, border);
}


void TextInput::update(View& view)
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
    m_bg_rect.clear();
    auto cursor_box = cursor_span->part(0).bbox();
    cursor_box.w = view.screen_ratio().x;
    m_cursor_shape.add_rectangle(cursor_box);
    // Background rect
    m_bg_rect.add_rectangle(bbox(), m_outline_thickness);
}


void TextInput::draw(View& view)
{
    auto rect = m_layout.bbox();
    auto pos = position() + Vec2f{m_padding - rect.x, m_padding - rect.y};
    m_bg_rect.draw(view, {0, 0});
    m_layout.draw(view, pos);
    m_cursor_shape.draw(view, pos);
}


void TextInput::handle(View& view, const KeyEvent& ev)
{
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

    update(view);
    view.refresh();
}


void TextInput::handle(View& view, const CharEvent& ev)
{
    auto ch = to_utf8(ev.code_point);
    m_text.insert(m_cursor, ch);
    m_cursor += ch.size();
    update(view);
    view.refresh();
}


void TextInput::handle(View& view, const MouseBtnEvent& ev)
{
    Widget::handle(view, ev);
}


util::Rect_f TextInput::bbox() const
{
    auto rect = m_layout.bbox();
    rect.enlarge(m_padding);
    rect.x = position().x;
    rect.y = position().y;
    return rect;
}


}} // namespace xci::widgets
