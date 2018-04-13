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

#include <xci/widgets/Theme.h>
#include <xci/text/Layout.h>

namespace xci {
namespace widgets {

using namespace xci::text;

class Icon {
public:
    explicit Icon(Theme& theme = Theme::default_theme());

    void set_icon(IconId icon_id);
    void set_text(const std::string& text);
    void set_size(float size);
    void set_color(const graphics::Color& color);

    void resize(const graphics::View& target);
    void draw(graphics::View& view, const util::Vec2f& pos);

    util::Rect_f bbox() const;

private:
    void refresh();

private:
    IconId m_icon_id = IconId::None;
    std::string m_text;
    Layout m_layout;
    Theme& m_theme;
    bool m_needs_refresh = false;
};


}} // namespace xci::widgets

#endif // XCI_WIDGETS_ICON_H
