// term.cpp created on 2018-07-09, part of XCI toolkit
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

#include "Term.h"
#include "log.h"
#include <xci/config.h>

#include <unistd.h>

#ifdef XCI_WITH_TINFO
#include <curses.h>
#include <term.h>
#endif

namespace xci {
namespace util {

using namespace log;

static_assert(int(Term::Color::Black) == COLOR_BLACK, "curses color black");
static_assert(int(Term::Color::Red) == COLOR_RED, "curses color red");
static_assert(int(Term::Color::Green) == COLOR_GREEN, "curses color green");
static_assert(int(Term::Color::Yellow) == COLOR_YELLOW, "curses color yellow");
static_assert(int(Term::Color::Blue) == COLOR_BLUE, "curses color blue");
static_assert(int(Term::Color::Magenta) == COLOR_MAGENTA, "curses color magenta");
static_assert(int(Term::Color::Cyan) == COLOR_CYAN, "curses color cyan");
static_assert(int(Term::Color::White) == COLOR_WHITE, "curses color white");


Term& Term::stdout_instance()
{
    static Term term(STDOUT_FILENO);
    return term;
}


Term& Term::stderr_instance()
{
    static Term term(STDERR_FILENO);
    return term;
}


Term::Term(int fd)
{
    // Do not even try if not TTY (ie. pipes)
    if (isatty(fd) != 1)
        return;
    // Setup terminfo
    int err = 0;
    if (setupterm(nullptr, fd, &err) != OK)
        return;
    // All ok
    m_fd = fd;
}


// Note that this cannot be implemented with variadic template,
// because the arguments must not be evaluated unless is_initialized() is true
#define TERM_APPEND(...) Term(*this, is_initialized() ? tparm(__VA_ARGS__) : "")

Term Term::fg(Term::Color color) const
{
    return TERM_APPEND(set_a_foreground, static_cast<int>(color));
}

Term Term::bg(Term::Color color) const
{
    return TERM_APPEND(set_a_background, static_cast<int>(color));
}

Term Term::bold() const { return TERM_APPEND(enter_bold_mode); }
Term Term::normal() const { return TERM_APPEND(exit_attribute_mode); }


std::ostream& operator<<(std::ostream& os, const Term& term)
{
    os << term.seq();
    return os;
}


std::string Term::format_cb(const format_impl::Context& ctx)
{
    if (ctx.placeholder == "bold")      return bold().seq();
    if (ctx.placeholder == "normal")    return normal().seq();
    if (ctx.placeholder == "black")     return black().seq();
    if (ctx.placeholder == "red")       return red().seq();
    if (ctx.placeholder == "green")     return green().seq();
    if (ctx.placeholder == "yellow")    return yellow().seq();
    if (ctx.placeholder == "blue")      return blue().seq();
    if (ctx.placeholder == "magenta")   return magenta().seq();
    if (ctx.placeholder == "cyan")      return cyan().seq();
    if (ctx.placeholder == "white")     return white().seq();

    return format_impl::print_placeholder(ctx);
}


}} // namespace xci::util
