// Label.cpp created on 2018-06-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
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


Label::Label(Theme& theme, const std::string& string, TextFormat format)
    : Label(theme)
{
    set_string(string, format);
}


void Label::resize(View& view)
{
    view.finish_draw();
    TextMixin::_resize(view);
    auto rect = m_layout.bbox();
    apply_padding(rect, view);
    set_size(rect.size());
    set_baseline(-rect.y);
    Widget::resize(view);
}


void Label::update(View& view, State state)
{
    TextMixin::_update(view);
}


void Label::draw(View& view)
{
    auto pop_offset = view.push_offset(position());
    auto rect = m_layout.bbox();
    const auto padding = padding_fb(view);
    FramebufferCoords pos = {padding.x - rect.x, padding.y - rect.y};
    TextMixin::_draw(view, pos);
}


} // namespace xci::widgets
