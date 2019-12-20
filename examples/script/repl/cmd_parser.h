// cmd_parser.h created on 2019-12-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_REPL_CMD_PARSER_H
#define XCI_SCRIPT_REPL_CMD_PARSER_H

#include "context.h"

namespace xci::script::repl {

void parse_command(const std::string& line, Context& ctx);

}  // namespace xci::script::repl

#endif // include guard
