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

namespace xci {
namespace widgets {

using namespace xci::graphics;
using namespace xci::text;


Button::Button(const std::string &string, Font& font)
    : m_bg_rect(Color(10, 20, 40), Color(180, 180, 0)),
      m_text("A button?", font)
{
    m_bg_rect.add_rectangle({-0.3f, -0.1f, 0.6, 0.2f}, 0.01);
    m_text.set_size(0.07);
    m_text.set_color(Color::White());
}


void Button::draw(graphics::View& view, const util::Vec2f& pos)
{
    m_bg_rect.draw(view, pos);
    m_text.draw(view, {-0.166f, 0.025});
}


}} // namespace xci::widgets
