// Icon.cpp created on 2018-04-10, part of XCI toolkit
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

#include "Icon.h"
#include <xci/util/string.h>

namespace xci {
namespace widgets {

using xci::util::to_utf8;


Icon::Icon(Theme& theme) : m_theme(theme) {}


void Icon::set_icon(IconId icon_id)
{
    m_icon_id = icon_id;
    m_needs_refresh = true;
}


void Icon::set_text(const std::string& text)
{
    m_text = text;
    m_needs_refresh = true;
}


void Icon::set_size(float size)
{
    m_layout.set_default_font_size(size);
    m_needs_refresh = true;
}


void Icon::set_color(const graphics::Color& color)
{
    m_layout.set_default_color(color);
}


void Icon::update(const graphics::View& target)
{
    if (m_needs_refresh)
        refresh();
    m_layout.typeset(target);
}


void Icon::draw(graphics::View& view, const util::Vec2f& pos)
{
    m_layout.draw(view, pos);
}


util::Rect_f Icon::bbox() const
{
    return m_layout.bbox();
}


void Icon::refresh()
{
    m_layout.clear();
    m_layout.set_font(&m_theme.icon_font());
    m_layout.set_offset({0, 0.125f});
    m_layout.add_word(to_utf8(m_theme.icon_codepoint(m_icon_id)));
    m_layout.reset_offset();
    m_layout.set_font(&m_theme.font());
    m_layout.add_space();
    m_layout.add_word(m_text);
}


}} // namespace xci::widgets
