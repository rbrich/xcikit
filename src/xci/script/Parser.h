// Parser.h created on 2019-05-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_PARSER_H
#define XCI_SCRIPT_PARSER_H

#include "ast/AST.h"
#include "Source.h"

#include <string_view>

namespace xci::script {


class Parser {
public:
    explicit Parser(const SourceManager& src_man) : m_source_manager(src_man) {}

    void parse(SourceId src_id, ast::Module& mod);

#ifndef NDEBUG
    /// Check that the grammar is built correctly.
    /// This is useful only during development.
    /// \returns 0 if grammar is OK
    static size_t analyze_grammar();
#endif

private:
    const SourceManager& m_source_manager;
};


} // namespace xci::script

#endif // include guard
