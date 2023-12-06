// Label.h created on 2018-06-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_WIDGETS_LABEL_H
#define XCI_WIDGETS_LABEL_H

#include <xci/widgets/Widget.h>
#include <xci/text/Text.h>

namespace xci::widgets {


class Label: public Widget, public Padded, public text::TextMixin {
public:
    using TextFormat = text::TextFormat;
    explicit Label(Theme& theme);
    explicit Label(Theme& theme, const std::string &string, TextFormat format = TextFormat::Plain);

    void resize(View& view) override;
    void update(View& view, State state) override;
    void draw(View& view) override;
};


} // namespace xci::widgets

#endif // XCI_WIDGETS_LABEL_H
