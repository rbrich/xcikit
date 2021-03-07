// EditBuffer.cpp created on 2021-02-27 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "EditBuffer.h"
#include "string.h"

namespace xci::core {


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


bool EditBuffer::move_to_home()
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
