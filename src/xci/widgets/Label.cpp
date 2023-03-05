// Label.cpp created on 2018-06-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Label.h"
#include <xci/text/Markup.h>

namespace xci::widgets {

using namespace text;
using graphics::FramebufferCoords;


Label::Label(Theme& theme)
    : Widget(theme)
{
    m_layout.set_default_font(&theme.base_font());
}


Label::Label(Theme& theme, const std::string& string)
    : Label(theme)
{
    m_layout.add_word(string);
}


void Label::set_string(const std::string& string)
{
    m_layout.clear();
    parse_plain(m_layout, string);
    m_need_typeset = true;
}


void Label::set_markup_string(const std::string& string)
{
    m_layout.clear();
    Markup(m_layout, string);
    m_need_typeset = true;
}


void Label::set_color(graphics::Color color)
{
    m_layout.set_default_color(color);
    m_need_typeset = true;
}


void Label::resize(View& view)
{
    view.finish_draw();
    m_need_typeset = false;
    m_layout.typeset(view);
    m_layout.update(view);
    auto rect = m_layout.bbox();
    apply_padding(rect, view);
    set_size(rect.size());
    set_baseline(-rect.y);
    Widget::resize(view);
}


void Label::update(View& view, State state)
{
    if (m_need_typeset) {
        m_need_typeset = false;
        m_layout.typeset(view);
        m_layout.update(view);
    }
}


void Label::draw(View& view)
{
    auto pop_offset = view.push_offset(position());
    auto rect = m_layout.bbox();
    const auto padding = padding_fb(view);
    FramebufferCoords pos = {padding.x - rect.x, padding.y - rect.y};
    m_layout.draw(view, pos);
}


} // namespace xci::widgets
