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


Icon::Icon(Theme& theme) : m_theme(theme)
{
    m_layout.set_default_font(&theme.icon_font());
}


void Icon::set_icon(IconId icon_id)
{
    m_icon_id = icon_id;
    m_layout.clear();
    m_layout.add_word(to_utf8(m_theme.icon_codepoint(icon_id)));
}


void Icon::set_size(float size)
{
    m_layout.set_default_font_size(size);
}


void Icon::set_color(const graphics::Color& color)
{
    m_layout.set_default_color(color);
}


void Icon::resize(const graphics::View& target)
{
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


}} // namespace xci::widgets
