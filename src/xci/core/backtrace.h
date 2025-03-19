// backtrace.h created on 2025-03-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_BACKTRACE_H
#define XCI_CORE_BACKTRACE_H

#include <string>

namespace xci::core {


/// Get backtrace for current thread. C++ symbols are demangled if possible.
/// \param file_lines  Enable file and line information. Implemented only on Windows.
/// \return String with formatted backtrace, in format similar to backtrace_symbols(3).
std::string get_backtrace(bool file_lines = false);


} // namespace xci::core

#endif  // include guard
