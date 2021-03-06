// EditLine.cpp created on 2021-02-26 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "EditLine.h"
#include "TermCtl.h"
#include "string.h"
#include "log.h"

#include <string>
#include <unistd.h>

namespace xci::core {


void EditLine::open_history_file(const fs::path& path)
{
    m_history_file.open(path, std::ios::in | std::ios::app);
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
            if (di.alt)
                // Alt + key  -> ignored
                continue;
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
                case TermCtl::Key::UnicodeChar:
                    m_edit_buffer.insert(to_utf8(di.unicode));
                    break;
                default:
                    // other control keys -> ignored
                    continue;
            }
            write(tout.move_to_column(prompt_end).seq());
            auto cursor = prompt_end + tout.stripped_length(m_edit_buffer.content_upto_cursor());
            write_highlight(m_edit_buffer.content_view(), m_edit_buffer.cursor());
            write(tout.clear_line_to_end().move_to_column(cursor).seq());
        }
    });

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


} // namespace xci::core
