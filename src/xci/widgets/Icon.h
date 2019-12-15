// Icon.h created on 2018-04-10, part of XCI toolkit
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

#ifndef XCI_WIDGETS_ICON_H
#define XCI_WIDGETS_ICON_H

#include <xci/widgets/Widget.h>
#include <xci/text/Layout.h>

namespace xci::widgets {


class Icon: public Widget {
public:
    explicit Icon(Theme& theme) : Widget(theme) {}

    void set_icon(IconId icon_id);
    void set_text(const std::string& text);
    void set_font_size(float size);
    void set_icon_color(const graphics::Color& color);
    void set_color(const graphics::Color& color);

    void resize(View& view) override;
    void update(View& view, State state) override;
    void draw(View& view) override;

private:
    IconId m_icon_id = IconId::None;
    graphics::Color m_icon_color = theme().color(ColorId::Default);
    std::string m_text;
    text::Layout m_layout;
    bool m_needs_refresh = false;
};


} // namespace xci::widgets

#endif // XCI_WIDGETS_ICON_H
