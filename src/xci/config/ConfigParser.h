// ConfigParser.h created on 2023-11-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CONFIG_PARSER_H
#define XCI_CONFIG_PARSER_H

#include <string>
#include <filesystem>

namespace xci::config {

namespace fs = std::filesystem;


/// The config format:
/// ```
/// bool_item false   // true/false
/// int_item 1
/// float_item 2.3
/// string_item "abc\n"  // quotes are required, supports C-style escape sequences
/// group {
///   value 1
///   subgroup { foo 42; bar "baz" }  // semicolon is used as a delimiter for multiple items on same line
/// }
/// ```
/// Whitespace is required between item name and value. Value must start on same line as name.
class ConfigParser {
public:
    virtual ~ConfigParser() = default;

    bool parse_file(const fs::path& path);
    bool parse_string(const std::string& str);

    // visitor callbacks

    virtual void name(const std::string& name) = 0;
    virtual void group(bool begin) = 0;

    virtual void bool_value(bool value) = 0;
    virtual void int_value(int64_t value) = 0;
    virtual void float_value(double value) = 0;
    virtual void string_value(std::string value) = 0;
};


}  // namespace xci::config

#endif  // XCI_CONFIG_PARSER_H
