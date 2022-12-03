// Label.cpp created on 2018-06-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Label.h"

namespace xci::widgets {

using graphics::FramebufferCoords;


Label::Label(Theme& theme)
    : Widget(theme)
{
    m_text.set_font(theme.font());
}


Label::Label(Theme& theme, const std::string& string)
    : Label(theme)
{
    m_text.set_fixed_string(string);
}


void Label::resize(View& view)
{
    view.finish_draw();
    m_text.resize(view);
    auto rect = m_text.layout().bbox();
    rect.enlarge(view.to_fb(m_padding));
    set_size(rect.size());
    set_baseline(-rect.y);
    Widget::resize(view);
}


void Label::draw(View& view)
{
    auto pop_offset = view.push_offset(position());
    auto rect = m_text.layout().bbox();
    const auto padding = view.to_fb(m_padding);
    FramebufferCoords pos = {padding - rect.x, padding - rect.y};
    m_text.draw(view, pos);
}


} // namespace xci::widgets
