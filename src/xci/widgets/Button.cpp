// Button.cpp created on 2018-03-21, part of XCI toolkit
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

#include "Button.h"
#include <xci/text/Markup.h>

namespace xci {
namespace widgets {

using namespace xci::graphics;
using namespace xci::text;


Button::Button(const std::string &string)
    : m_bg_rect(Color(10, 20, 40), Color(180, 180, 0))
{
    m_layout.set_default_font(&theme().font());
    Markup markup(m_layout);
    markup.parse(string);
}


void Button::set_decoration_color(const graphics::Color& fill,
                                  const graphics::Color& border)
{
    m_bg_rect = Shape(fill, border);
}


void Button::set_text_color(const graphics::Color& color)
{
    m_layout.set_default_color(color);
}


void Button::update(const graphics::View& target)
{
    m_layout.typeset(target);
    m_bg_rect.clear();
    m_bg_rect.add_rectangle(bbox(), m_outline_thickness);
}


void Button::draw(graphics::View& view)
{
    m_bg_rect.draw(view, position() + m_padding);
    m_layout.draw(view, position() + m_padding);
}


void Button::handle(View& view, const MouseBtnEvent& ev)
{
    if (ev.action == Action::Press && ev.button == MouseButton::Left) {
        if (m_click_cb && bbox().contains(ev.pos - position())) {
            m_click_cb(view);
            view.refresh();
        }
    }
}


util::Rect_f Button::bbox() const
{
    auto rect = m_layout.bbox();
    rect.enlarge(m_padding);
    return rect;
}


}} // namespace xci::widgets
