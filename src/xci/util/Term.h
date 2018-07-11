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

namespace xci {
namespace util {

class Term {
public:
    Term() = default;
    explicit Term(int fd): m_fd(fd) {}

    bool is_tty() const;

private:
    int m_fd = 1;  // stdout
};

}} // namespace xci::util

#endif //XCI_UTIL_TERM_H
