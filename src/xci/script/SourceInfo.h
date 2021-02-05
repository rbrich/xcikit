// SourceInfo.h created on 2019-12-25 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_SOURCEINFO_H
#define XCI_SCRIPT_SOURCEINFO_H

#include <cstddef>

namespace xci::script {


// Offset into source code, used to print error context
// Possibly better name: SourceLocation, SourceContext
struct SourceInfo {
    // both line_number and column start at 1
    size_t line_number = 0;
    size_t column = 0;
    const char* line_begin = nullptr;
    const char* line_end = nullptr;
    const char* source = nullptr;

    template <typename Input, typename Position>
    void load(const Input &in, const Position& pos) {
        line_begin = in.begin_of_line(pos);
        line_end = in.end_of_line(pos);
        line_number = pos.line;
        column = pos.column;
        source = in.source();
    }
};


} // namespace xci::script

#endif // include guard
