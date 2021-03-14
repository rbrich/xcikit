// EditBuffer.cpp created on 2021-02-27 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "EditBuffer.h"
#include "string.h"
#include <algorithm>
#include <wctype.h>

namespace xci::core {


void EditBuffer::set_cursor(size_t absolute_position)
{
    m_cursor = std::min(absolute_position, m_content.size());
}


void EditBuffer::insert(std::string_view text)
{
    m_content.insert(m_cursor, text);
    m_cursor += text.size();
}


bool EditBuffer::delete_left()
{
    if (m_cursor == 0)
        return false;
    auto pos = m_content.crbegin() + m_content.size() - m_cursor;
    auto prev = m_content.crbegin() + m_content.size() - utf8_prev(pos);
    m_content.erase(prev, m_cursor - prev);
    m_cursor = prev;
    return true;
}


bool EditBuffer::delete_right()
{
    if (m_cursor >= m_content.size())
        return false;
    auto next = utf8_next(m_content.cbegin() + m_cursor) - m_content.cbegin();
    m_content.erase(m_cursor, next - m_cursor);
    return true;
}


bool EditBuffer::move_left()
{
    if (m_cursor == 0)
        return false;
    auto pos = m_content.crbegin() + m_content.size() - m_cursor;
    m_cursor = m_content.crbegin() + m_content.size() - utf8_prev(pos);
    return true;
}


bool EditBuffer::move_right()
{
    if (m_cursor >= m_content.size())
        return false;
    m_cursor = utf8_next(m_content.cbegin() + m_cursor) - m_content.cbegin();
    return true;
}


bool EditBuffer::move_up()
{
    if (m_cursor == 0)
        return false;
    // find end of the previous line
    auto nl_end = m_content.rfind('\n', m_cursor - 1);
    if (nl_end == std::string::npos)  // no newline before cursor
        return false;
    auto col_in_line = utf8_length(m_content.substr(nl_end, m_cursor - nl_end));
    // find start of the previous line
    auto nl_start = m_content.rfind('\n', nl_end - 1);
    if (nl_start == std::string::npos)
        // moving to the first line
        m_cursor = utf8_offset(m_content, std::min(nl_end, col_in_line - 1));
    else
        // jump to a corresponding column (if the line is long enough)
        m_cursor = nl_start + utf8_offset({m_content.data() + nl_start, m_content.size() - nl_start},
                std::min(nl_end - nl_start, col_in_line));
    return true;
}


bool EditBuffer::move_down()
{
    if (m_cursor >= m_content.size())
        return false;
    // find start of the next line
    auto nl_start = m_content.find('\n', m_cursor);
    if (nl_start == std::string::npos)  // no newline after cursor
        return false;
    // compute the current col_in_line
    size_t col_in_line = 0;
    if (m_cursor != 0) {
        auto nl_prev = m_content.rfind('\n', m_cursor - 1);
        col_in_line = (nl_prev == std::string::npos)
                ? utf8_length(m_content.substr(0, m_cursor))
                : utf8_length(m_content.substr(nl_prev + 1, m_cursor - nl_prev - 1));
    }
    // find end of the next line
    auto nl_end = m_content.find('\n', nl_start + 1);
    if (nl_end == std::string::npos)
        // moving to the last line
        nl_end = m_content.size();
    m_cursor = nl_start + utf8_offset({m_content.data() + nl_start, m_content.size() - nl_start},
            std::min(nl_end - nl_start, col_in_line + 1));
    return true;
}


bool EditBuffer::move_to_line_beginning()
{
    if (m_cursor == 0)
        return false;
    auto nl = m_content.rfind('\n', m_cursor - 1);
    if (nl == std::string::npos)  // no newline before cursor
        m_cursor = 0;
    else if (nl + 1 == m_cursor)  // cursor already on the char right after newline
        return false;
    else
        m_cursor = nl + 1;
    return true;
}


bool EditBuffer::move_to_line_end()
{
    if (m_cursor >= m_content.size())
        return false;
    auto nl = m_content.find('\n', m_cursor);
    if (nl == std::string::npos)  // no newline before cursor
        m_cursor = m_content.size();
    else if (nl == m_cursor)  // cursor already on the line end
        return false;
    else
        m_cursor = nl;
    return true;
}


bool EditBuffer::move_to_beginning()
{
    if (m_cursor == 0)
        return false;
    m_cursor = 0;
    return true;
}


bool EditBuffer::move_to_end()
{
    if (m_cursor >= m_content.size())
        return false;
    m_cursor = m_content.size();
    return true;
}


static bool is_word_char(wint_t c)
{
    return iswalnum(c);
}


bool EditBuffer::skip_word_left()
{
    if (m_cursor == 0)
        return false;
    // first skip non-word chars, then word chars
    while (!is_word_char_left_of_cursor() && move_left()) {}
    while (is_word_char_left_of_cursor() && move_left()) {}
    return true;
}


bool EditBuffer::skip_word_right()
{
    if (m_cursor >= m_content.size())
        return false;
    // first skip non-word chars, then word chars
    while (!is_word_char(utf8_codepoint(m_content.c_str() + m_cursor)) && move_right()) {}
    while (is_word_char(utf8_codepoint(m_content.c_str() + m_cursor)) && move_right()) {}
    return true;
}


bool EditBuffer::delete_word_left()
{
    const auto orig_cursor = m_cursor;
    if (!skip_word_left())
        return false;
    m_content.erase(m_cursor, orig_cursor - m_cursor);
    return true;
}


bool EditBuffer::delete_word_right()
{
    const auto orig_cursor = m_cursor;
    if (!skip_word_right())
        return false;
    m_content.erase(orig_cursor, m_cursor - orig_cursor);
    m_cursor = orig_cursor;
    return true;
}


bool EditBuffer::is_word_char_left_of_cursor() const
{
    const auto rlast = m_content.crbegin() + m_content.size();
    const auto new_cursor = rlast - utf8_prev(rlast - m_cursor);
    return is_word_char(utf8_codepoint(m_content.c_str() + new_cursor));
}


} // namespace xci::core
