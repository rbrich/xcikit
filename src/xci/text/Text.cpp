// Text.cpp created on 2018-03-02, part of XCI toolkit
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

#include "Text.h"
#include "Markup.h"

#include <xci/graphics/Sprites.h>

namespace xci::text {

using namespace graphics;
using namespace core;


Text::Text(Font& font, const std::string &string, Format format)
{
    m_layout.set_default_font(&font);
    set_string(string, format);
}


static void parse_plain(Layout& layout, const std::string& s)
{
    auto it = s.begin();
    auto word = s.end();
    auto finish_word = [&] {
        if (word == s.end())
            return;
        layout.add_word(std::string{word, it});
        word = s.end();
    };
    while (it != s.end()) {
        switch (*it) {
            case '\t':
                finish_word();
                layout.add_tab();
                break;
            case '\n':
                finish_word();
                layout.finish_line();
                break;
            default:
                if (word == s.end())
                    word = it;
                break;
        }
        ++it;
    }
    finish_word();
}


void Text::set_string(const std::string& string, Format format)
{
    m_layout.clear();
    switch (format) {
        case Format::None:
            m_layout.add_word(string);
            break;
        case Format::Plain:
            parse_plain(m_layout, string);
            break;
        case Format::Markup:
            Markup(m_layout, string);
            break;
    }
    m_need_typeset = true;
}


void Text::set_width(ViewportUnits width)
{
    m_layout.set_default_page_width(width);
    m_need_typeset = true;
}


void Text::set_font(Font& font)
{
    m_layout.set_default_font(&font);
    m_need_typeset = true;
}


void Text::set_font_size(ViewportUnits size)
{
    m_layout.set_default_font_size(size);
    m_need_typeset = true;
}


void Text::set_color(const graphics::Color& color)
{
    m_layout.set_default_color(color);
    m_need_typeset = true;
}


void Text::resize(graphics::View& view)
{
    m_layout.typeset(view);
    m_layout.update(view);
    m_need_typeset = false;
}


void Text::update(graphics::View& view)
{
    view.finish_draw();
    if (m_need_typeset) {
        m_layout.typeset(view);
        m_need_typeset = false;
    }
    m_layout.update(view);
}


void Text::draw(graphics::View& view, const ViewportCoords& pos)
{
    m_layout.draw(view, pos);
}


} // namespace xci::text
