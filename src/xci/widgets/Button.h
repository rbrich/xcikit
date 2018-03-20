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

#include <xci/graphics/Rectangles.h>
#include <xci/text/Font.h>
#include <xci/text/Text.h>

namespace xci {
namespace widgets {


class Button {
public:
    Button(const std::string &string, text::Font& font);

    void draw(graphics::View& view, const util::Vec2f& pos);

private:
    graphics::Rectangles m_bg_rect;
    text::Text m_text;
};


}} // namespace xci::widgets

#endif // XCI_WIDGETS_BUTTON_H
