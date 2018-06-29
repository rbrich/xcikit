// Text.cpp created on 2018-03-02, part of XCI toolkit
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

#include "Text.h"
#include "Markup.h"

#include <xci/graphics/Sprites.h>

namespace xci {
namespace text {

using namespace graphics;
using namespace util;


Text::Text(const std::string &string, Font& font)
{
    m_layout.set_default_font(&font);
    set_string(string);
}


void Text::set_string(const std::string& string)
{
    m_layout.clear();
    Markup markup(m_layout);
    markup.parse(string);
}


void Text::set_fixed_string(const std::string& string)
{
    m_layout.clear();
    m_layout.add_word(string);
}


void Text::resize(graphics::View& view)
{
    m_layout.typeset(view);
}


void Text::draw(graphics::View& view, const util::Vec2f& pos)
{
    m_layout.draw(view, pos);
}


void Text::resize_draw(View& view, const Vec2f& pos)
{
    m_layout.typeset(view);
    m_layout.draw(view, pos);
}


}} // namespace xci::text
