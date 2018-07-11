// term.h created on 2018-07-09, part of XCI toolkit
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

#ifndef XCI_UTIL_TERM_H
#define XCI_UTIL_TERM_H

#include <string>
#include <ostream>

namespace xci {
namespace util {


class Term {
public:
    Term() { initialize(1 /* stdout */); }
    explicit Term(int fd) { initialize(fd); }

    // Initialize the terminal
    void initialize(int fd);
    bool is_initialized() const { return m_fd != -1; }

    // Following methods are appending the capability codes
    // to a copy of Term instance, which can then be send to stream

    // foreground
    Term red() const;

    // background
    Term on_blue() const;

    // mode
    Term bold() const;
    Term normal() const;  // reset all attributes

    // Output cached seq
    friend std::ostream& operator<<(std::ostream& os, const Term& term);

private:
    // Copy Term and append seq to new instance
    Term(const Term& term, const char* seq) : m_fd(term.m_fd), m_seq(term.m_seq + seq) {}

private:
    int m_fd = -1;  // terminal attached to this FD
    std::string m_seq;  // cached capability sequences
};

}} // namespace xci::util

#endif //XCI_UTIL_TERM_H
