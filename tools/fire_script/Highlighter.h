// Highlighter.h created on 2021-03-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCIKIT_HIGHLIGHTER_H
#define XCIKIT_HIGHLIGHTER_H

#include <xci/core/TermCtl.h>
#include <string_view>
#include <string>

namespace xci::script::tool {

namespace parser { struct Node; }


class Highlighter {
public:
    using Color = xci::core::TermCtl::Color;
    using Mode = xci::core::TermCtl::Mode;
    struct HighlightColor {
        Color fg = Color::Default;
        Mode mode = Mode::Normal;
        Color bg = Color::Default;
    };

    Highlighter(xci::core::TermCtl& t) : m_term(t) {}

    struct HlResult {
        std::string highlighted_input;
        bool is_open;  // true if the input has open bracket or is otherwise expecting some more input (ENTER will add a new line)
    };
    HlResult highlight(std::string_view input, unsigned cursor);

private:
    void switch_color(const HighlightColor& from, const HighlightColor& to);
    HighlightColor highlight_node(const parser::Node& node,
                                  const HighlightColor& prev_color,
                                  unsigned cursor, bool hl_bracket = false);

    xci::core::TermCtl& m_term;
    std::string m_output;
    bool m_open_bracket = false;
};


} // namespace xci::script::tool

#endif // include guard
