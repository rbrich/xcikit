// Label.h created on 2018-06-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCIKIT_LABEL_H
#define XCIKIT_LABEL_H

#include <xci/widgets/Widget.h>
#include <xci/text/Layout.h>

namespace xci::widgets {


class Label: public Widget, public Padded {
public:
    explicit Label(Theme& theme);
    explicit Label(Theme& theme, const std::string &string);

    void set_string(const std::string& string);
    void set_markup_string(const std::string& string);

    void set_color(graphics::Color color);

    text::Layout& layout() { return m_layout; }

    void resize(View& view) override;
    void update(View& view, State state) override;
    void draw(View& view) override;

private:
    text::Layout m_layout;
    bool m_need_typeset = true;
};


} // namespace xci::widgets

#endif // XCIKIT_LABEL_H
