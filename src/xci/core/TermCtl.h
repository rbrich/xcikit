// TermCtl.h created on 2018-07-09, part of XCI toolkit
// Copyright 2018 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef XCI_CORE_TERM_H
#define XCI_CORE_TERM_H

#include "format.h"
#include <string>
#include <ostream>

namespace xci {
namespace core {

// Sends control codes and escape sequences to controlling terminal
// or does nothing if the output stream is not connected to TTY.

// FIXME: switch cur_term, free resources (del_curterm)

class TermCtl {
public:
    // Static instances for standard streams
    static TermCtl& stdout_instance();
    static TermCtl& stderr_instance();

    // Constructor for custom streams
    explicit TermCtl(int fd);

    // Is the output stream connected to TTY?
    bool is_tty() const { return m_fd != -1; }

    // Following methods are appending the capability codes
    // to a copy of TermCtl instance, which can then be send to stream

    enum class Color { Black, Red, Green, Yellow, Blue, Magenta, Cyan, White };

    // foreground
    TermCtl fg(Color color) const;
    TermCtl black() const { return fg(Color::Black); }
    TermCtl red() const { return fg(Color::Red); }
    TermCtl green() const { return fg(Color::Green); }
    TermCtl yellow() const { return fg(Color::Yellow); }
    TermCtl blue() const { return fg(Color::Blue); }
    TermCtl magenta() const { return fg(Color::Magenta); }
    TermCtl cyan() const { return fg(Color::Cyan); }
    TermCtl white() const { return fg(Color::White); }

    // background
    TermCtl bg(Color color) const;
    TermCtl on_black() const { return bg(Color::Black); }
    TermCtl on_red() const { return bg(Color::Red); }
    TermCtl on_green() const { return bg(Color::Green); }
    TermCtl on_yellow() const { return bg(Color::Yellow); }
    TermCtl on_blue() const { return bg(Color::Blue); }
    TermCtl on_magenta() const { return bg(Color::Magenta); }
    TermCtl on_cyan() const { return bg(Color::Cyan); }
    TermCtl on_white() const { return bg(Color::White); }

    // mode
    TermCtl bold() const;
    TermCtl underline() const;
    TermCtl overline() const;
    TermCtl normal() const;  // reset all attributes

    // Output cached seq
    const std::string& seq() const { return m_seq; }
    friend std::ostream& operator<<(std::ostream& os, const TermCtl& term);

    template<typename ...Args>
    std::string format(const char *fmt, Args... args) {
        return fun_format(fmt, [this](const format_impl::Context& ctx) {
            return format_cb(ctx);
        }, args...);
    }

private:
    // Copy TermCtl and append seq to new instance
    TermCtl(const TermCtl& term, const std::string& seq) : m_fd(term.m_fd), m_seq(term.m_seq + seq) {}

    std::string format_cb(const format_impl::Context& ctx);

private:
    int m_fd = -1;  // terminal attached to this FD
    std::string m_seq;  // cached capability sequences
};

}} // namespace xci::core

#endif //XCI_CORE_TERM_H
