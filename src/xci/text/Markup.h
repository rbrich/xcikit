// Markup.h created on 2018-03-10 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_MARKUP_H
#define XCI_TEXT_MARKUP_H

#include "Layout.h"

namespace xci::text {

/// Minimal markup language. Any similarity to HTML is purely coincidental.
///
/// Supported tags:
/// * `<br>` - line break ('\n')
/// * `<p>` or `\n\n` - paragraph break - not a pair element!
/// * `<tab>` or `\t` - tabulator
/// * `<b>`, `</b>` - bold, not bold
/// * `<i>`, `</i>` - italic, not italic
/// * `<c:#ABC>` - set RGB color (3 hex digits)
/// * `</c>` - reset color to default
/// * `<s:name>` ... `</s:name>` - named span

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


/// Even more minimalistic parser
/// Supported control:
/// * `\n` - line break
/// * `\t` - tabulator
void parse_plain(Layout& layout, const std::string& s);


} // namespace xci::text

#endif // XCI_TEXT_MARKUP_H
