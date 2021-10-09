// Markup.h created on 2018-03-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_MARKUP_H
#define XCI_TEXT_MARKUP_H

#include "Layout.h"

namespace xci::text {

/// Minimal XML-like markup language.
///
/// Supported tokens:
/// * `<br>` - line break ('\n')
/// * `<tab>` - tabulator ('\t')
/// * `<b>` ... `</b>` - bold
/// * `<i>` ... `</i>` - italic
/// * `<name>` ... `</name>` - named span

class Markup {
public:
    explicit Markup(Layout &layout) : m_layout(layout) {}
    explicit Markup(Layout &layout, const std::string &s)
        : m_layout(layout) { parse(s); }

    bool parse(const std::string &s);

    Layout& get_layout() { return m_layout; }

private:
    Layout& m_layout;
};


} // namespace xci::text

#endif // XCI_TEXT_MARKUP_H
