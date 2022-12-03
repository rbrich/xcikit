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
    void set_content(std::string content) { m_content = std::move(content); m_cursor = m_content.size(); }

    /// Cursor position, valid range is:
    /// * from 0 - at beginning, before first char
    /// * to content.size() - at end, after last char
    size_t cursor() const { return m_cursor; }
    void set_cursor(size_t absolute_position);

    const std::string& content() const { return m_content; }
    std::string_view content_view() const { return m_content; }
    bool empty() const { return m_content.empty(); }

    std::string_view content_upto_cursor() const { return std::string_view(m_content).substr(0, m_cursor); }
    std::string_view content_from_cursor() const { return std::string_view(m_content).substr(m_cursor); }

    void insert(std::string_view text);

    // Keyboard actions - returns true when buffer was modified (action succeeded)
    bool delete_left();             // Backspace
    bool delete_right();            // Delete
    bool move_left();               // Left
    bool move_right();              // Right
    bool move_up();                 // Up
    bool move_down();               // Down
    bool move_to_line_beginning();  // Home
    bool move_to_line_end();        // End
    bool move_to_beginning();       // PgUp, Alt + Home
    bool move_to_end();             // PgDown, Alt + End
    bool skip_word_left();          // Alt + Left
    bool skip_word_right();         // Alt + Right
    bool delete_word_left();        // Alt + Backspace
    bool delete_word_right();       // Alt + Delete

    bool is_word_char_left_of_cursor() const;

private:
    std::string m_content;
    size_t m_cursor = 0;
};


} // namespace xci::core

#endif // include guard
