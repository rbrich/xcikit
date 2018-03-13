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


void Text::draw(View& target, const Vec2f& pos)
{
    if (!m_parsed) {
        Markup markup(m_layout);
        markup.parse(m_string);
        m_parsed = true;
    }
    m_layout.draw(target, pos);
}


}} // namespace xci::text
