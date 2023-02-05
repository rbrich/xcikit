// Icon.cpp created on 2018-04-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Icon.h"
#include <xci/core/string.h>

namespace xci::widgets {

using xci::core::to_utf8;
using namespace xci::graphics;
using namespace xci::text;


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


void Icon::set_font_size(graphics::VariUnits size)
{
    m_layout.set_default_font_size(size);
    m_needs_refresh = true;
}


void Icon::set_icon_color(graphics::Color color)
{
    m_icon_color = color;
}


void Icon::set_color(graphics::Color color)
{
    m_layout.set_default_color(color);
    m_needs_refresh = true;
}


void Icon::resize(View& view)
{
    view.finish_draw();
    if (m_needs_refresh) {
        // Refresh
        m_layout.clear();
        m_layout.set_font(&theme().icon_font());
        m_layout.begin_span("icon");
        m_layout.set_offset({0_fb, 0.125f * view.to_fb(m_layout.default_style().size())});
        m_layout.add_word(to_utf8(theme().icon_codepoint(m_icon_id)));
        m_layout.end_span("icon");
        m_layout.reset_offset();
        m_layout.set_font(&theme().base_font());
        m_layout.add_space();
        m_layout.add_word(m_text);
        m_needs_refresh = false;
    }
    m_layout.typeset(view);
    m_layout.update(view);
    auto rect = m_layout.bbox();
    set_size(rect.size());
    set_baseline(-rect.y);
    Widget::resize(view);
}


void Icon::update(View& view, State state)
{
    view.finish_draw();
    m_layout.get_span("icon")->adjust_style([this, &state](Style& s) {
        s.set_color(state.focused ? theme().color(ColorId::Focus) : m_icon_color);
    });
    m_layout.update(view);
}


void Icon::draw(View& view)
{
    auto rect = m_layout.bbox();
    m_layout.draw(view, position() - rect.top_left());
}


} // namespace xci::widgets
