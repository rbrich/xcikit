// Button.h created on 2018-03-21, part of XCI toolkit
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

#ifndef XCI_WIDGETS_BUTTON_H
#define XCI_WIDGETS_BUTTON_H

#include "Theme.h"
#include <xci/graphics/Shape.h>
#include <xci/graphics/Color.h>
#include <xci/text/Font.h>
#include <xci/text/Text.h>

namespace xci {
namespace widgets {


class Button {
public:
    explicit Button(const std::string &string, Theme& theme = Theme::default_theme());

    void set_font_size(float size) { m_layout.set_default_font_size(size); }
    void set_padding(float padding) { m_padding = padding; }
    void set_outline_thickness(float thickness) { m_outline_thickness = thickness; }

    void set_decoration_color(const graphics::Color& fill, const graphics::Color& border);
    void set_text_color(const graphics::Color& color);

    void resize(const graphics::View& target);
    void draw(graphics::View& view, const util::Vec2f& pos);

    util::Rect_f bbox() const;

private:
    Theme& m_theme;
    graphics::Shape m_bg_rect;
    text::Layout m_layout;
    float m_padding = 0.02f;
    float m_outline_thickness = 0.005f;
};


}} // namespace xci::widgets

#endif // XCI_WIDGETS_BUTTON_H
