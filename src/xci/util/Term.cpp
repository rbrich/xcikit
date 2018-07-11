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


void Term::initialize(int fd)
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


Term Term::red() const { return Term(*this, is_initialized() ? tparm(set_a_foreground, COLOR_RED) : ""); }
Term Term::on_blue() const { return Term(*this, is_initialized() ? tparm(set_a_background, COLOR_BLUE) : ""); }
Term Term::bold() const { return Term(*this, is_initialized() ? enter_bold_mode : ""); }
Term Term::normal() const { return Term(*this, is_initialized() ? exit_attribute_mode : ""); }


std::ostream& operator<<(std::ostream& os, const Term& term)
{
    os << term.m_seq;
    return os;
}


}} // namespace xci::util
