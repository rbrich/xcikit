// TermCtl.cpp created on 2018-07-09, part of XCI toolkit
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
//
// References:
// - https://en.wikipedia.org/wiki/POSIX_terminal_interface
// - https://en.wikipedia.org/wiki/ANSI_escape_code

#include "TermCtl.h"
#include "log.h"
#include <xci/config.h>

#include <unistd.h>
#include <termios.h>
#include <cassert>

#ifdef XCI_WITH_TINFO
#include <term.h>
#include <cassert>

#endif

namespace xci::core {

using namespace log;

// When building without TInfo, emit ANSI escape sequences directly
#ifndef XCI_WITH_TINFO
static constexpr auto enter_bold_mode = "\033[1m";
static constexpr auto enter_underline_mode = "\033[4m";
static constexpr auto exit_attribute_mode = "\033[0m";
static constexpr auto set_a_foreground = "\033[3{}m";
static constexpr auto set_a_background = "\033[4{}m";
static constexpr auto parm_up_cursor = "\033[{}A";  // move cursor up N lines
static constexpr auto clr_eos = "\033[J";  // clear screen from cursor down
inline constexpr const char* tparm(const char* seq) { return seq; }
template<typename ...Args>
inline std::string tparm(const char* seq, Args... args) { return format(seq, args...); }
#endif // XCI_WITH_TINFO

// not in Terminfo DB:
static constexpr auto enter_overline_mode = "\033[53m";


TermCtl& TermCtl::stdout_instance()
{
    static TermCtl term(STDOUT_FILENO);
    return term;
}


TermCtl& TermCtl::stderr_instance()
{
    static TermCtl term(STDERR_FILENO);
    return term;
}


TermCtl::TermCtl(int fd)
{
    // Do not even try if not TTY (ie. pipes)
    if (isatty(fd) != 1)
        return;
#ifdef XCI_WITH_TINFO
    // Setup terminfo
    int err = 0;
    if (setupterm(nullptr, fd, &err) != 0)
        return;
#endif
    // All ok
    m_fd = fd;
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


TermCtl TermCtl::move_up(int n_lines) const { return TERM_APPEND(parm_up_cursor, n_lines); }

TermCtl TermCtl::clear_screen_down() const { return TERM_APPEND(clr_eos); }


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
}


int TermCtl::raw_getch()
{
    char buf = 0;
    with_raw_mode([&buf] {
        while (read(0, &buf, 1) < 0 && (errno == EINTR || errno == EAGAIN));
    });
    return buf;
}


} // namespace xci::core
