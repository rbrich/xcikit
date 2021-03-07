// EditLine.cpp created on 2021-02-26 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

// Key binding reference:
// * https://www.gnu.org/savannah-checkouts/gnu/bash/manual/bash.html#Command-Line-Editing
// * https://www.gnu.org/savannah-checkouts/gnu/bash/manual/bash.html#Commands-For-Moving

#include "EditLine.h"
#include "TermCtl.h"
#include "string.h"
#include "log.h"

#include <string>
#include <unistd.h>

namespace xci::core {


void EditLine::open_history_file(const fs::path& path)
{
    m_history_file.open(path, std::ios::in);
    if (m_history_file) {
        std::string line;
        while (std::getline(m_history_file, line)) {
            switch (line[0]) {
                case ' ':
                case '~':
                    m_history.push_back(line.substr(1));
                    break;
                case '|':
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
        auto pos = begin;
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


const char* EditLine::input(std::string_view prompt)
{
    auto& tin = TermCtl::stdin_instance();
    auto& tout = TermCtl::stdout_instance();

    auto prompt_end = tout.stripped_length(prompt);
    write("\r");
    write(prompt);
    m_edit_buffer.clear();

    tin.with_raw_mode([this, prompt_end] {
        auto& tout = TermCtl::stdout_instance();
        bool done = false;
        bool need_more_input = true;
        while (!done) {
            // obtain more input from terminal
            if (need_more_input) {
                if (!read_input())
                    break;
                need_more_input = false;
            }

            // decode the head of input data
            TermCtl::DecodedInput di;
            {
                std::unique_lock lock(m_input_mutex);
                di = tout.decode_input(m_input_buffer);
                if (di.input_len == 0) {
                    need_more_input = true;
                    continue;
                }
                m_input_buffer.erase(0, di.input_len);
            }

            // process the decoded input
            switch (di.mod) {
                case TermCtl::Modifier::None:
                    // normal
                    switch (di.key) {
                        case TermCtl::Key::Enter:
                            write("\n\r");
                            done = true;
                            continue;
                        case TermCtl::Key::Backspace:
                            if (!m_edit_buffer.delete_left())
                                continue;
                            break;
                        case TermCtl::Key::Delete:
                            if (!m_edit_buffer.delete_right())
                                continue;
                            break;
                        case TermCtl::Key::Home:
                            if (!m_edit_buffer.move_to_home())
                                continue;
                            break;
                        case TermCtl::Key::End:
                            if (!m_edit_buffer.move_to_end())
                                continue;
                            break;
                        case TermCtl::Key::Left:
                            if (!m_edit_buffer.move_left())
                                continue;
                            break;
                        case TermCtl::Key::Right:
                            if (!m_edit_buffer.move_right())
                                continue;
                            break;
                        case TermCtl::Key::Up:
                            if (!history_previous())
                                continue;
                            break;
                        case TermCtl::Key::Down:
                            if (!history_next())
                                continue;
                            break;
                        case TermCtl::Key::UnicodeChar:
                            m_edit_buffer.insert(to_utf8(di.unicode));
                            break;
                        default:
                            // other keys -> ignored
                            continue;
                    }
                    break;
                case TermCtl::Modifier::Alt:
                    // with Alt
                    switch (di.key) {
                        case TermCtl::Key::Enter:
                            // multi-line edit - next line
                            m_edit_buffer.insert("\n");
                            break;
                        case TermCtl::Key::Backspace:
                            if (!m_edit_buffer.delete_word_left())
                                continue;
                            break;
                        case TermCtl::Key::Delete:
                            if (!m_edit_buffer.delete_word_right())
                                continue;
                            break;
                        case TermCtl::Key::Home:
                            if (!m_edit_buffer.move_to_home())
                                continue;
                            break;
                        case TermCtl::Key::End:
                            if (!m_edit_buffer.move_to_end())
                                continue;
                            break;
                        case TermCtl::Key::Left:
                            if (!m_edit_buffer.skip_word_left())
                                continue;
                            break;
                        case TermCtl::Key::Right:
                            if (!m_edit_buffer.skip_word_right())
                                continue;
                            break;
                        case TermCtl::Key::UnicodeChar:
                            switch (di.unicode) {
                                case 'b':
                                    // Alt-b (backward-word)
                                    if (!m_edit_buffer.skip_word_left())
                                        continue;
                                    break;
                                case 'f':
                                    // Alt-f (forward-word)
                                    if (!m_edit_buffer.skip_word_right())
                                        continue;
                                    break;
                            }
                            break;
                        default:
                            // other keys -> ignored
                            continue;
                    }
                    break;
                case TermCtl::Modifier::Ctrl:
                    // with Ctrl
                    if (di.key == TermCtl::Key::UnicodeChar) {
                        switch (tolower(di.unicode)) {
                            case 'a':
                                // Ctrl-a (beginning-of-line)
                                if (!m_edit_buffer.move_to_home())
                                    continue;
                                break;
                            case 'e':
                                // Ctrl-e (end-of-line)
                                if (!m_edit_buffer.move_to_end())
                                    continue;
                                break;
                            case 'b':
                                // Ctrl-b (backward-char)
                                if (!m_edit_buffer.move_left())
                                    continue;
                                break;
                            case 'f':
                                // Ctrl-f (forward-char)
                                if (!m_edit_buffer.move_right())
                                    continue;
                                break;
                            default:
                                // other keys -> ignored
                                continue;
                        }
                    } else {
                        // other keys -> ignored
                        continue;
                    }
                    break;
                case TermCtl::Modifier::CtrlAlt:
                    // ignored
                    continue;
            }
            write(tout.move_to_column(prompt_end).seq());
            auto cursor = prompt_end + tout.stripped_length(m_edit_buffer.content_upto_cursor());
            write_highlight(m_edit_buffer.content_view(), m_edit_buffer.cursor());
            write(tout.clear_line_to_end().move_to_column(cursor).seq());
        }
    });

    // reset history cursor in case we were editing an item from history
    m_history_cursor = -1;
    m_history_orig_buffer.clear();

    return m_edit_buffer.content().c_str();
}


void EditLine::feed_input(std::string_view data)
{
    {
        std::unique_lock lock(m_input_mutex);
        m_input_buffer += data;
    }
    m_input_cv.notify_one();
}


bool EditLine::read_input()
{
    std::unique_lock lock(m_input_mutex);

    // read from FD (stdin or custom)
    if (m_input_fd >= 0) {
        char buf[100];
        ssize_t rc;
        while ((rc = ::read(m_input_fd, buf, sizeof(buf))) < 0
               && (errno == EINTR || errno == EAGAIN));
        if (rc > 0) {
            // ok
            m_input_buffer.append(buf, rc);
            return true;
        }
        if (rc < 0) {
            log::error("read: {m}");
            return false;
        }
        // 0 = eof
        return false;
    }

    // read from custom feed
    m_input_cv.wait(lock, [this]{ return !m_input_buffer.empty(); });
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
        m_history_cursor = m_history.size();
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


} // namespace xci::core
