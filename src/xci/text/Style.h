// Style.h created on 2018-03-18, part of XCI toolkit
// Copyright 2018, 2019 Radek Brich
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

#ifndef XCI_TEXT_STYLE_H
#define XCI_TEXT_STYLE_H

#include "Font.h"
#include <xci/graphics/Color.h>
#include <xci/graphics/View.h>

namespace xci::text {

using graphics::ViewportUnits;
using namespace graphics::unit_literals;


class Style {
public:
    void clear();

    void set_font(Font* font) { m_font = font; }
    Font* font() const { return m_font; }

    void set_size(ViewportUnits size) { m_size = size; }
    ViewportUnits size() const { return m_size; }

    void set_color(const graphics::Color &color) { m_color = color; }
    const graphics::Color& color() const { return m_color; }

private:
    Font* m_font = nullptr;
    ViewportUnits m_size = 0.05_vp;
    graphics::Color m_color = graphics::Color::White();
};


} // namespace xci::text

#endif // XCI_TEXT_STYLE_H
