// TermCtl.h created on 2018-07-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_TERM_H
#define XCI_CORE_TERM_H

#include "format.h"
#include <string>
#include <ostream>

namespace xci::core {

// Sends control codes and escape sequences to controlling terminal
// or does nothing if the output stream is not connected to TTY.

// FIXME: switch cur_term, free resources (del_curterm)

class TermCtl {
public:
    enum class Mode {
        Auto,
        Always,
        Never,
    };

    // Static instance for standard output
    static TermCtl& stdout_instance(Mode mode = Mode::Auto);
    static TermCtl& stderr_instance(Mode mode = Mode::Auto);

    // Constructor for custom streams
    explicit TermCtl(int fd, Mode mode = Mode::Auto);
    ~TermCtl();

    // Is the output stream connected to TTY?
    [[nodiscard]] bool is_tty() const { return m_state != State::NoTTY; }

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

    // cursor movement
    TermCtl move_up() const;
    TermCtl move_up(unsigned n_lines) const;
    TermCtl move_down() const;
    TermCtl move_down(unsigned n_lines) const;
    TermCtl move_left() const;
    TermCtl move_left(unsigned n_cols) const;
    TermCtl move_right() const;
    TermCtl move_right(unsigned n_cols) const;

    // clear screen content
    TermCtl clear_screen_down() const;

    TermCtl soft_reset() const;

    // Output cached seq
    const std::string& seq() const { return m_seq; }
    friend std::ostream& operator<<(std::ostream& os, const TermCtl& term);

    template<typename ...Args>
    std::string format(const char *fmt, Args... args) {
        return fun_format(fmt, [this](const format_impl::Context& ctx) {
            return format_cb(ctx);
        }, args...);
    }

    // Temporarily change terminal mode to RAW mode
    // (no echo, no buffering, no special processing)
    // NOTE: Signal processing is enabled, so Ctrl-C still works
    void with_raw_mode(const std::function<void()>& cb);
    std::string raw_input();

private:
    // Copy TermCtl and append seq to new instance
    TermCtl(const TermCtl& term, const std::string& seq)
        : m_state(term.m_state == State::NoTTY ? State::NoTTY : State::CopyOk)
        , m_seq(term.m_seq + seq) {}

    std::string format_cb(const format_impl::Context& ctx);

private:
    enum class State {
        NoTTY,      // initialization failed
        InitOk,     // main instance (it will reset the term when destroyed)
        CopyOk,     // a copy created by chained method
    };
    State m_state = State::NoTTY;
    std::string m_seq;  // cached capability sequences

#ifdef _WIN32
    unsigned long m_std_handle = 0;
    unsigned long m_orig_out_mode = 0;
#endif
};

} // namespace xci::core

#endif //XCI_CORE_TERM_H
