// Label.cpp created on 2018-06-23, part of XCI toolkit
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

#include "Label.h"

namespace xci {
namespace widgets {


Label::Label(const std::string& string)
{
    m_layout.set_default_font(&theme().font());
    m_layout.add_word(string);
}


void Label::resize(View& view)
{
    m_layout.typeset(view);
    auto rect = m_layout.bbox();
    rect.enlarge(m_padding);
    set_size(rect.size());
}


void Label::draw(View& view, State state)
{
    view.push_offset(position());
    auto rect = m_layout.bbox();
    auto pos = Vec2f{m_padding - rect.x,
                     m_padding - rect.y};
    m_layout.draw(view, pos);
    view.pop_offset();
}


}} // namespace xci::widgets
