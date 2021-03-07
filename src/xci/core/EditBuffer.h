// EditBuffer.h created on 2021-02-27 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_EDITBUFFER_H
#define XCI_CORE_EDITBUFFER_H

#include <string>
#include <string_view>

namespace xci::core {


/// Internal buffer for text-editing components
/// Tracks cursor position and supports various editing operations at the cursor
class EditBuffer {
public:
    EditBuffer() = default;
    explicit EditBuffer(std::string initial_content)
        : m_content(std::move(initial_content)), m_cursor(m_content.size()) {}

    void clear() { m_content.clear(); m_cursor = 0; }
    void set_content(std::string content) { m_content = std::move(content); m_cursor = content.size(); }

    /// Cursor position, valid range is:
    /// * from 0 - at beginning, before first char
    /// * to content.size() - at end, after last char
    unsigned long cursor() const { return m_cursor; }
    void set_cursor(unsigned long absolute_position) { m_cursor = std::max(absolute_position, m_content.size()); }

    const std::string& content() const { return m_content; }
    std::string_view content_view() const { return m_content; }

    std::string_view content_upto_cursor() const { return std::string_view(m_content).substr(0, m_cursor); }
    std::string_view content_from_cursor() const { return std::string_view(m_content).substr(m_cursor); }

    void insert(std::string_view text);

    // Keyboard actions - returns true when buffer was modified (action succeeded)
    bool delete_left();         // backspace
    bool delete_right();        // delete
    bool move_left();           // left
    bool move_right();          // right
    bool move_to_home();        // home
    bool move_to_end();         // end
    bool skip_word_left();      // alt + left
    bool skip_word_right();     // alt + right
    bool delete_word_left();    // alt + backspace
    bool delete_word_right();   // alt + delete

    bool is_word_char_left_of_cursor() const;

private:
    std::string m_content;
    unsigned long m_cursor = 0;
};


} // namespace xci::core

#endif // include guard
