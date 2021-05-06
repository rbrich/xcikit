// raw_string.h created on 2021-05-06 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_PARSER_RAW_STRING_H
#define XCI_SCRIPT_PARSER_RAW_STRING_H

#include <string>

namespace xci::script {


std::string strip_raw_string(std::string&& content);


} // namespace xci::script


#endif  // include guard
