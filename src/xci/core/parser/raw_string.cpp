// raw_string.cpp created on 2021-05-06 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "raw_string.h"
#include <cstddef>  // std::ptrdiff_t

namespace xci::core::parser {


/// Skip spaces and tabs, success on a newline, fail on any other char
/// \returns 0 = fail, >0 success (a number of leading blanks including the newline)
template <typename Iter>
static size_t count_leading_ws(Iter&& begin, Iter&& end)
{
    size_t leading_ws = 0;
    auto it = begin;
    while (it != end) {
        if (*it == '\n')
            return ++leading_ws;
        if (isblank(*it)) {
            ++leading_ws;
            ++it;
            continue;
        }
        return 0;
    }
    return leading_ws;
}


std::string strip_raw_string(std::string&& content)
{
    const size_t leading_ws = count_leading_ws(content.begin(), content.end());
    const size_t trailing_ws = count_leading_ws(content.rbegin(), content.rend());
    if (leading_ws == 0 || trailing_ws == 0)
        return std::move(content);

    // remove leading and trailing whitespace
    content.erase(0, leading_ws);
    content.erase(content.size() - trailing_ws);

    // check uniform indentation
    const size_t indentation = trailing_ws - 1;
    if (indentation == 0)
        return std::move(content);

    auto it = content.cbegin();
    const auto end = content.cbegin() + std::ptrdiff_t(content.size());
    while (it != end) {
        size_t need_indent = indentation;
        while (need_indent > 0 && isblank(*it)) {
            --need_indent;
            ++it;
        }
        if (need_indent != 0)
            return std::move(content);  // line not indented enough, abort
        while (it != end && *it++ != '\n')
            ;
    }

    // remove uniform indentation
    it = content.cbegin();
    std::string out;
    while (it != end) {
        auto newline = it;
        while (newline != end && *newline++ != '\n')
            ;
        out.append(it + std::ptrdiff_t(indentation), newline);
        it = newline;
    }
    return out;
}


} // namespace xci::core::parser
