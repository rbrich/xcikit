// EditLine.cpp created on 2021-02-26 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

// Key binding reference:
// * https://www.gnu.org/savannah-checkouts/gnu/bash/manual/bash.html#Command-Line-Editing
// * https://www.gnu.org/savannah-checkouts/gnu/bash/manual/bash.html#Commands-For-Moving

#include "EditLine.h"
#include "string.h"
#include "log.h"

#include <string>

namespace xci::core {


void EditLine::open_history_file(const fs::path& path)
{
    // load previous history
    m_history_file.open(path, std::ios::in);
    if (m_history_file) {
        std::string line;
        while (std::getline(m_history_file, line)) {
            switch (line[0]) {
                case ' ':
                case '~':  // ~ starts multi-line item
                    m_history.push_back(line.substr(1));
                    break;
                case '|':  // append another line to a multi-line item
                    line[0] = '\n';
                    m_history.back().append(line);
                    break;
            }
        }
        m_history_file.close();
    }
    // reopen for appending
    m_history_file.open(path, std::ios::out | std::ios::app);
}


void EditLine::add_history(std::string_view input)
{
    // don't add if same as the last item
    if (!m_history.empty() && m_history.back() == input)
        return;
    // add to memory
    m_history.emplace_back(input);
    // append to history file, if open
    if (!m_history_file)
        return;
    // file format: first char on each line indicates single- or multi-line item:
    // ' ' single-line
    // '~' multi-line first line
    // '|' multi-line following lines
    if (std::find(input.cbegin(), input.cend(), '\n') == input.end()) {
        // single-line
        m_history_file << ' ' << input << '\n';
        m_history_file.flush();
    } else {
        // multi-line
        auto begin = input.begin();
        auto end = input.end();
        decltype(begin) pos;
        do {
            pos = std::find(begin, end, '\n');
            if (begin == input.begin())
                m_history_file << '~';
            else
                m_history_file << '|';
            const auto line = input.substr(begin - input.begin(), pos - begin);
            m_history_file << line << '\n';
            begin = pos + 1;
        } while (pos != end);
        m_history_file.flush();
    }
}


bool EditLine::process_key(TermCtl::Key key)
{
    switch (key) {
        case TermCtl::Key::Backspace:   return m_edit_buffer.delete_left();
        case TermCtl::Key::Delete:      return m_edit_buffer.delete_right();
        case TermCtl::Key::Home:        return m_edit_buffer.move_to_line_beginning();
        case TermCtl::Key::End:         return m_edit_buffer.move_to_line_end();
        case TermCtl::Key::Left:        return m_edit_buffer.move_left();
        case TermCtl::Key::Right:       return m_edit_buffer.move_right();
        case TermCtl::Key::Up:          return history_previous();
        case TermCtl::Key::Down:        return history_next();
        case TermCtl::Key::PageUp:      return m_edit_buffer.move_to_beginning();
        case TermCtl::Key::PageDown:    return m_edit_buffer.move_to_end();
        default:
            // other keys -> ignored
            return false;
    }
}


bool EditLine::process_alt_key(TermCtl::Key key)
{
    switch (key) {
        case TermCtl::Key::Enter:
            // multi-line edit - next line
            if (is_multiline()) {
                m_edit_buffer.insert("\n");
                return true;
            }
            return false;
        case TermCtl::Key::Backspace:   return m_edit_buffer.delete_word_left();
        case TermCtl::Key::Delete:      return m_edit_buffer.delete_word_right();
        case TermCtl::Key::Home:        return m_edit_buffer.move_to_line_beginning();
        case TermCtl::Key::End:         return m_edit_buffer.move_to_line_end();
        case TermCtl::Key::Left:        return m_edit_buffer.skip_word_left();
        case TermCtl::Key::Right:       return m_edit_buffer.skip_word_right();
        case TermCtl::Key::Up:          return m_edit_buffer.move_up();
        case TermCtl::Key::Down:        return m_edit_buffer.move_down();
        default:
            // other keys -> ignored
            return false;
    }
}


bool EditLine::process_alt_char(char32_t unicode)
{
    switch (unicode) {
        case 'b':
            // Alt-b (backward-word)
            return m_edit_buffer.skip_word_left();
        case 'f':
            // Alt-f (forward-word)
            return m_edit_buffer.skip_word_right();
        case 'd':
            // Alt-d (kill-word)
            return m_edit_buffer.delete_word_right();
        default:
            return false;
    }
}


bool EditLine::process_ctrl_char(char32_t unicode)
{
    switch (unicode) {
        case 'c':
            m_state = State::ControlBreak;
            return true; // force redraw before exiting, to position cursor to the bottom
        case 'd':
            // Ctrl-d (end-of-file) - when the line is empty
            if (m_edit_buffer.empty()) {
                m_state = State::ControlBreak;
                return true;
            }
            // Ctrl-d (delete-char)
            return m_edit_buffer.delete_right();
        case 'a':
            // Ctrl-a (beginning-of-line)
            return m_edit_buffer.move_to_line_beginning();
        case 'e':
            // Ctrl-e (end-of-line)
            return m_edit_buffer.move_to_line_end();
        case 'b':
            // Ctrl-b (backward-char)
            return m_edit_buffer.move_left();
        case 'f':
            // Ctrl-f (forward-char)
            return m_edit_buffer.move_right();
        case 'p':
            // Ctrl-p (previous-history)
            return history_previous();
        case 'n':
            // Ctrl-n (next-history)
            return history_next();
        default:
            // other keys -> ignored
            return false;
    }
}


auto EditLine::input(std::string_view prompt) -> std::pair<bool, std::string_view>
{
    start_input(prompt);

    auto& tin = TermCtl::stdin_instance();
    tin.with_raw_mode([this] {
        if (!read_input())
            return;
        for (;;) {
            process_input();
            if (m_state == State::Continue)
                continue;
            if (m_state == State::NeedMoreInputData) {
                // obtain more input from terminal
                if (!read_input())
                    return;
                continue;
            }
            // Finished, ControlBreak
            return;
        }
    });

    return finish_input();
}


void EditLine::start_input(std::string_view prompt)
{
    auto& tout = TermCtl::stdout_instance();

    m_prompt_len = tout.stripped_width(prompt);
    write("\r");
    write(prompt);
    flush();
    m_edit_buffer.clear();
    m_edit_continue_nl = false;
    m_cursor_line = 0;
}


void EditLine::process_input()
{
    auto& tout = TermCtl::stdout_instance();

    // decode the head of input data
    TermCtl::DecodedInput di;
    {
        di = tout.decode_input(m_input_buffer);
        if (di.input_len == 0) {
            m_state = State::NeedMoreInputData;
            return;
        }
        m_input_buffer.erase(0, di.input_len);
    }

    // process the decoded input
    m_state = State::Continue;
    switch (di.mod.normalized_flags()) {
        case TermCtl::Modifier::None:
            // normal
            switch (di.key) {
                case TermCtl::Key::Enter:
                    if (is_multiline() && m_edit_continue_nl) {
                        // multi-line edit - continue to next line (unclosed bracket etc.)
                        m_edit_buffer.insert("\n");
                        break;
                    }
                    // finish input
                    m_state = State::Finished;
                    break;  // force redraw before exiting, to position cursor to the bottom
                case TermCtl::Key::UnicodeChar:
                    m_edit_buffer.insert(to_utf8(di.unicode));
                    break;
                default:
                    if (!process_key(di.key))
                        return;  // Continue
            }
            break;
        case TermCtl::Modifier::Alt:
            // with Alt
            if (di.key == TermCtl::Key::UnicodeChar) {
                if (!process_alt_char(di.unicode))
                    return;  // Continue
            } else {
                if (!process_alt_key(di.key))
                    return;  // Continue
            }
            break;
        case TermCtl::Modifier::Ctrl:
            // with Ctrl
            if (di.key == TermCtl::Key::UnicodeChar) {
                if (!process_ctrl_char(di.unicode))
                    return;  // Continue
            } else {
                // Ctrl + Left etc. mirrors Alt + Left etc. (Windows-style shortcuts)
                if (!process_alt_key(di.key))
                    return;  // Continue
            }
            break;
        case (TermCtl::Modifier::Ctrl | TermCtl::Modifier::Alt):
        default:
            // ignored
            return;  // Continue
    }
    if (m_cursor_line != 0) {
        // move back to prompt line
        write(tout.move_up(m_cursor_line).seq());
        m_cursor_line = 0;
    }
    write(tout.move_to_column(m_prompt_len).clear_screen_down().seq());
    auto cursor = tout.stripped_width(m_edit_buffer.content_upto_cursor());

    // Optionally highlight the content
    std::string_view content;  // pointer to content, either original or highlighted
    std::string content_store;  // helper for owning the highlighted string
    if (m_highlight_cb) {
        auto r = m_highlight_cb(m_edit_buffer.content_view(), (unsigned) m_edit_buffer.cursor());
        m_edit_continue_nl = r.is_open;
        content_store = std::move(r.hl_data);
        content = content_store;
    } else {
        content = m_edit_buffer.content_view();
    }

    if (is_multiline()) {
        bool cursor_found = false;
        unsigned cursor_row = 0;
        unsigned cursor_col = 0;
        for (const auto line : xci::core::split(content, '\n')) {
            if (line.data() != content.data()) {
                // not the first line
                write("\r\n" + std::string(m_prompt_len, ' '));
                ++m_cursor_line;
            }
            write(line);
            if (!cursor_found) {
                auto part_len = tout.stripped_width(line);
                if (cursor > part_len) {
                    cursor -= part_len + 1;  // add 1 for '\n'
                } else {
                    // cursor position found
                    cursor_row = m_cursor_line;
                    cursor_col = m_prompt_len + cursor;
                    cursor_found = true;
                }
            }
        }
        if (m_state == State::Continue) {
            // move cursor to position, only if not
            if (cursor_row != m_cursor_line) {
                write(tout.move_up(m_cursor_line - cursor_row).seq());
                m_cursor_line = cursor_row;
            }
            write(tout.move_to_column(cursor_col).seq());
        } else {
            // after Enter or Ctrl+C, leave cursor at the bottom
            if (m_state != State::ControlBreak) {
                // Enter - add line break before following output
                write("\r\n");
            }
        }
    } else {
        // no multi-line
        write(content);
        write(tout.move_to_column(m_prompt_len + cursor).seq());
        if (m_state == State::Finished)
            write("\r\n");
    }
    flush();
}


auto EditLine::finish_input() -> std::pair<bool, std::string_view>
{
    // reset history cursor in case we were editing an item from history
    m_history_cursor = -1;
    m_history_orig_buffer.clear();

    return {m_state != State::ControlBreak,m_edit_buffer.content_view()};
}


bool EditLine::feed_input(std::string_view data)
{
    m_input_buffer += data;
    for (;;) {
        process_input();
        if (m_state == State::Continue)
            continue;
        if (m_state == State::NeedMoreInputData)
            return true;
        // Finished, ControlBreak
        return false;
    }
}


bool EditLine::read_input()
{
    auto& tin = TermCtl::stdin_instance();
    auto data = tin.input();
    if (data.empty())
        return false;  // eof or error
    m_input_buffer += data;
    return true;
}


bool EditLine::history_previous()
{
    if (m_history_cursor == 0)
        return false;  // already at the first item
    if (m_history_cursor == -1) {
        // not yet browsing history -> start now
        if (m_history.empty())
            return false;
        m_history_cursor = (int) m_history.size();
        m_history_orig_buffer = m_edit_buffer.content();
    } else {
        // replace history item in case it was edited
        m_history[m_history_cursor] = m_edit_buffer.content();
    }
    -- m_history_cursor;
    m_edit_buffer.set_content(m_history[m_history_cursor]);
    return true;
}


bool EditLine::history_next()
{
    if (m_history_cursor == -1)
        return false;  // already out of history
    // replace history item in case it was edited
    m_history[m_history_cursor] = m_edit_buffer.content();
    // move to next item
    ++ m_history_cursor;
    if (size_t(m_history_cursor) >= m_history.size()) {
        // leaving history
        m_history_cursor = -1;
        m_edit_buffer.set_content(std::move(m_history_orig_buffer));
        return true;
    }
    m_edit_buffer.set_content(m_history[m_history_cursor]);
    return true;
}


void EditLine::write(std::string_view data)
{
    m_output_buffer += data;
}


void EditLine::flush()
{
    auto& tout = TermCtl::stdout_instance();
    tout.write(m_output_buffer);
    m_output_buffer.clear();
}


} // namespace xci::core
