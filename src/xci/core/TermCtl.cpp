// TermCtl.cpp created on 2018-07-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

// References:
// - https://en.wikipedia.org/wiki/POSIX_terminal_interface
// - https://en.wikipedia.org/wiki/ANSI_escape_code
// - https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences

#include "TermCtl.h"
#include <xci/compat/unistd.h>
#include <xci/config.h>

#ifdef _WIN32
    #include <windows.h>
    static_assert(sizeof(unsigned long) == sizeof(DWORD));
#else
    #include <termios.h>
#endif

#ifdef XCI_WITH_TINFO
    #include <term.h>
#endif

#include <cassert>

namespace xci::core {

#define ESC     "\033"
#define CSI     ESC "["

// When building without TInfo, emit ANSI escape sequences directly
#ifndef XCI_WITH_TINFO
static constexpr auto cursor_up = CSI "A";
static constexpr auto cursor_down = CSI "B";
static constexpr auto cursor_right = CSI "C";
static constexpr auto cursor_left = CSI "D";
static constexpr auto enter_bold_mode = CSI "1m";
static constexpr auto enter_underline_mode = CSI "4m";
static constexpr auto exit_attribute_mode = CSI "0m";
static constexpr auto set_a_foreground = CSI "3{}m";
static constexpr auto set_a_background = CSI "4{}m";
static constexpr auto parm_up_cursor = CSI "{}A";  // move cursor up N lines
static constexpr auto parm_down_cursor = CSI "{}B";  // move cursor down N lines
static constexpr auto parm_right_cursor = CSI "{}C";  // move cursor right N spaces
static constexpr auto parm_left_cursor = CSI "{}D";  // move cursor left N spaces
static constexpr auto clr_eos = CSI "J";  // clear screen from cursor down
inline constexpr const char* tparm(const char* seq) { return seq; }
template<typename ...Args>
inline std::string tparm(const char* seq, Args... args) { return format(seq, args...); }
#endif // XCI_WITH_TINFO

// not found in Terminfo DB:
static constexpr auto enter_overline_mode = CSI "53m";
static constexpr auto send_soft_reset = CSI "!p";


#ifdef _WIN32
// returns orig mode or bad_mode on failure
static constexpr unsigned long bad_mode = ~0ul;
static DWORD set_console_mode(DWORD hid, DWORD req_mode)
{
    HANDLE h = GetStdHandle(hid);
    if (h == INVALID_HANDLE_VALUE)
        return bad_mode;
    DWORD orig_mode = 0;
    if (!GetConsoleMode(h, &orig_mode))
        return bad_mode;
    if (!SetConsoleMode(h, orig_mode | req_mode))
        return bad_mode;
    return orig_mode;
}
static void reset_console_mode(DWORD hid, DWORD mode)
{
    HANDLE h = GetStdHandle(hid);
    if (h != INVALID_HANDLE_VALUE)
        SetConsoleMode(h, mode);
}
#endif


TermCtl& TermCtl::stdout_instance(Mode mode)
{
    static TermCtl term(STDOUT_FILENO, mode);
    return term;
}


TermCtl& TermCtl::stderr_instance(Mode mode)
{
    static TermCtl term(STDERR_FILENO, mode);
    return term;
}


TermCtl::TermCtl(int fd, Mode mode) : m_fd(fd)
{
#ifdef _WIN32
    assert(fd == STDOUT_FILENO || fd == STDERR_FILENO);
    m_std_handle = (fd == STDOUT_FILENO) ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE;
    m_orig_out_mode = set_console_mode(m_std_handle,
                         ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    if (m_orig_out_mode == bad_mode)
        return;
#else
    switch (mode) {
        case Mode::Auto:
            // Do not even try if not TTY (e.g. when piping to other command)
            if (isatty(fd) != 1)
                return;
            break;
        case Mode::Always:
            break;
        case Mode::Never:
            return;
    }


    #ifdef XCI_WITH_TINFO
    // Setup terminfo
    int err = 0;
    if (setupterm(nullptr, fd, &err) != 0)
        return;
    #endif
#endif

    m_state = State::InitOk;
}


TermCtl::~TermCtl() {
#ifdef _WIN32
    if (m_state == State::InitOk) {
        assert(m_orig_out_mode != bad_mode);
        reset_console_mode(m_std_handle, m_orig_out_mode);
    }
#else
    (void) 0;
#endif
}


// Note that this cannot be implemented with variadic template,
// because the arguments must not be evaluated unless is_initialized() is true
#define TERM_APPEND(...) TermCtl(*this, is_tty() ? tparm(__VA_ARGS__) : "")

TermCtl TermCtl::fg(TermCtl::Color color) const
{
    return TERM_APPEND(set_a_foreground, static_cast<int>(color));
}

TermCtl TermCtl::bg(TermCtl::Color color) const
{
    return TERM_APPEND(set_a_background, static_cast<int>(color));
}

TermCtl TermCtl::bold() const { return TERM_APPEND(enter_bold_mode); }
TermCtl TermCtl::underline() const { return TERM_APPEND(enter_underline_mode); }
TermCtl TermCtl::overline() const { return TERM_APPEND(enter_overline_mode); }
TermCtl TermCtl::normal() const { return TERM_APPEND(exit_attribute_mode); }

TermCtl TermCtl::move_up() const { return TERM_APPEND(cursor_up); }
TermCtl TermCtl::move_up(unsigned n_lines) const { return TERM_APPEND(parm_up_cursor, n_lines); }
TermCtl TermCtl::move_down() const { return TERM_APPEND(cursor_down); }
TermCtl TermCtl::move_down(unsigned n_lines) const { return TERM_APPEND(parm_down_cursor, n_lines); }
TermCtl TermCtl::move_left() const { return TERM_APPEND(cursor_left); }
TermCtl TermCtl::move_left(unsigned n_cols) const { return TERM_APPEND(parm_left_cursor, n_cols); }
TermCtl TermCtl::move_right() const { return TERM_APPEND(cursor_right); }
TermCtl TermCtl::move_right(unsigned n_cols) const { return TERM_APPEND(parm_right_cursor, n_cols); }

TermCtl TermCtl::clear_screen_down() const { return TERM_APPEND(clr_eos); }

TermCtl TermCtl::soft_reset() const { return TERM_APPEND(send_soft_reset); }


std::ostream& operator<<(std::ostream& os, const TermCtl& term)
{
    os << term.seq();
    return os;
}


std::string TermCtl::format_cb(const format_impl::Context& ctx)
{
    // mode
    if (ctx.placeholder == "bold")      return bold().seq();
    if (ctx.placeholder == "underline") return underline().seq();
    if (ctx.placeholder == "overline")  return overline().seq();
    if (ctx.placeholder == "normal")    return normal().seq();
    // foreground
    if (ctx.placeholder == "black")     return black().seq();
    if (ctx.placeholder == "red")       return red().seq();
    if (ctx.placeholder == "green")     return green().seq();
    if (ctx.placeholder == "yellow")    return yellow().seq();
    if (ctx.placeholder == "blue")      return blue().seq();
    if (ctx.placeholder == "magenta")   return magenta().seq();
    if (ctx.placeholder == "cyan")      return cyan().seq();
    if (ctx.placeholder == "white")     return white().seq();
    // background
    if (ctx.placeholder == "on_black")   return on_black().seq();
    if (ctx.placeholder == "on_red")     return on_red().seq();
    if (ctx.placeholder == "on_green")   return on_green().seq();
    if (ctx.placeholder == "on_yellow")  return on_yellow().seq();
    if (ctx.placeholder == "on_blue")    return on_blue().seq();
    if (ctx.placeholder == "on_magenta") return on_magenta().seq();
    if (ctx.placeholder == "on_cyan")    return on_cyan().seq();
    if (ctx.placeholder == "on_white")   return on_white().seq();
    // unknown placeholder - leave as is
    return format_impl::print_placeholder(ctx);
}


void TermCtl::with_raw_mode(const std::function<void()>& cb)
{
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE)
        return;
    DWORD orig_mode = 0;
    if (!GetConsoleMode(h, &orig_mode))
        return;
    if (!SetConsoleMode(h, ENABLE_VIRTUAL_TERMINAL_INPUT |
            ENABLE_PROCESSED_INPUT ))
        return;

    cb();

    SetConsoleMode(h, orig_mode);
#else
    struct termios origtc = {};
    if (tcgetattr(0, &origtc) < 0) {
        assert(!"tcgetattr failed");
        return;
    }

    struct termios newtc = origtc;
    cfmakeraw(&newtc);
    newtc.c_lflag |= ISIG;
    if (tcsetattr(0, TCSANOW, &newtc) < 0)  {
        assert(!"tcsetattr failed");
        return;
    }

    cb();

    if (tcsetattr(0, TCSANOW, &origtc) < 0) {
        assert(!"tcsetattr failed");
        return;
    }
#endif
}


std::string TermCtl::raw_input()
{
    char buf[20] {};
    with_raw_mode([&buf] {
        while (::read(STDIN_FILENO, buf, sizeof buf) < 0
           && (errno == EINTR || errno == EAGAIN));
    });
    return buf;
}


void TermCtl::print(const std::string& buf)
{
    ::write(m_fd, buf.data(), buf.size());
}


} // namespace xci::core
