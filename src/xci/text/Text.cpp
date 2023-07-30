// Text.cpp created on 2018-03-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Text.h"
#include "Markup.h"

namespace xci::text {

using namespace graphics;
using namespace core;


Text::Text(Font& font, const std::string &string, TextFormat format)
{
    m_layout.set_default_font(&font);
    set_string(string, format);
}


void TextMixin::set_string(const std::string& string, TextFormat format)
{
    m_layout.clear();
    switch (format) {
        case TextFormat::None:
            m_layout.add_word(string);
            break;
        case TextFormat::Plain:
            parse_plain(m_layout, string);
            break;
        case TextFormat::Markup:
            Markup(m_layout, string);
            break;
    }
    m_need_typeset = true;
}


void TextMixin::set_width(VariUnits width)
{
    m_layout.set_default_page_width(width);
    m_need_typeset = true;
}


void TextMixin::set_font(Font& font)
{
    m_layout.set_default_font(&font);
    m_need_typeset = true;
}


void TextMixin::set_font_size(VariUnits size, bool allow_scale)
{
    m_layout.set_default_font_size(size, allow_scale);
    m_need_typeset = true;
}


void TextMixin::set_font_style(FontStyle font_style)
{
    m_layout.set_default_font_style(font_style);
    m_need_typeset = true;
}


void TextMixin::set_font_weight(uint16_t weight)
{
    m_layout.set_default_font_weight(weight);
    m_need_typeset = true;
}


void TextMixin::set_color(graphics::Color color)
{
    m_layout.set_default_color(color);
    m_need_typeset = true;
}


void TextMixin::set_outline_radius(VariUnits radius)
{
    m_layout.set_default_outline_radius(radius);
    m_need_typeset = true;
}


void TextMixin::set_outline_color(graphics::Color color)
{
    m_layout.set_default_outline_color(color);
    m_need_typeset = true;
}


void TextMixin::set_tab_stops(std::vector<VariUnits> stops)
{
    m_layout.set_default_tab_stops(std::move(stops));
    m_need_typeset = true;
}


void TextMixin::set_alignment(Alignment alignment)
{
    m_layout.set_default_alignment(alignment);
    m_need_typeset = true;
}


void TextMixin::_resize(graphics::View& view)
{
    m_layout.typeset(view);
    m_layout.update(view);
    m_need_typeset = false;
}


void TextMixin::_update(graphics::View& view)
{
    if (m_need_typeset) {
        m_layout.typeset(view);
        m_need_typeset = false;
    }
    m_layout.update(view);
}


void TextMixin::_draw(graphics::View& view, VariCoords pos)
{
    m_layout.draw(view, pos);
}


} // namespace xci::text
