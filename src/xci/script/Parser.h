// Parser.h created on 2019-05-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_PARSER_H
#define XCI_SCRIPT_PARSER_H

#include "ast/AST.h"

#include <string_view>

namespace xci::script {


class Parser {
public:
    // TODO: support incremental parsing

    void parse(std::string_view input, ast::Module& mod);

#ifndef NDEBUG
    /// Check that the grammar is built correctly.
    /// This is useful only during development.
    /// \returns 0 if grammar is OK
    static int analyze_grammar();
#endif
};


} // namespace xci::script

#endif // include guard
