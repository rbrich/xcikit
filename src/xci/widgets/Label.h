// Label.h created on 2018-06-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCIKIT_LABEL_H
#define XCIKIT_LABEL_H

#include <xci/widgets/Widget.h>
#include <xci/text/Text.h>

namespace xci::widgets {

using namespace xci::graphics::unit_literals;


class Label: public Widget {
public:
    explicit Label(Theme& theme);
    explicit Label(Theme& theme, const std::string &string);

    text::Text& text() { return m_text; }

    void resize(View& view) override;
    void draw(View& view) override;

private:
    text::Text m_text;
    VariUnits m_padding = 1_vp;
};


} // namespace xci::widgets

#endif // XCIKIT_LABEL_H
