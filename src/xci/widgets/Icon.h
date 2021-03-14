// Icon.h created on 2018-04-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

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
