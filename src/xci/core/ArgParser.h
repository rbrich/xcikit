// ArgParser.h created on 2019-06-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_ARG_PARSER_H
#define XCI_CORE_ARG_PARSER_H

#include <string>
#include <utility>
#include <vector>
#include <functional>
#include <sstream>
#include <initializer_list>
#include <iostream>

namespace xci::core::argparser {


template <class T>
inline bool from_cstr(const char* s, T& value) {
    std::istringstream is(std::string{s});
    is >> value;
    return is.eof();
}

template <>
inline bool from_cstr<const char*>(const char* s, const char*& value) {
    value = s;
    return true;
}

struct DefaultHelp {};
constexpr DefaultHelp show_help{};

struct Option {
    using Setter = std::function<bool(const char* arg)>;

    /// \param desc     comma-separated short and long names,
    ///                 e.g. "-v, --verbose"
    Option(const char* desc, const char* help, Setter setter, int flags);

    Option(const char* desc, const char* help, DefaultHelp)
            : Option(desc, help, nullptr, FPrintHelp) {}

    Option(const char* desc, const char* help, Setter setter)
            : Option(desc, help, std::move(setter), 0) {}

    Option(const char* desc, const char* help, std::function<void()> flag_setter)
            : Option(desc, help, [&](const char* arg){ flag_setter(); return true; }, 0) {}

    template <class T>
    Option(const char* desc, const char* help, T& value)
            : Option(desc, help, [&value](const char* arg){ return from_cstr<T>(arg, value); }, 0) {}

    bool has_short(char arg) const;
    bool has_long(const char* arg) const;
    bool has_args() const { return m_args != 0; }
    bool is_positional() const { return m_flags & FPositional; }
    bool is_print_help() const { return m_flags & FPrintHelp; }
    bool can_receive() const { return (m_flags & FDots) || m_received < m_args; }
    const char* desc() const { return m_desc; }
    const char* help() const { return m_help; }
    std::string usage() const;
    std::string formatted_desc(size_t width) const;

    bool operator() (const char* arg) { ++m_received; return m_setter(arg); }

private:
    struct NamePos {
        // e.g. ", --example 123"
        // - pos = 2 (skip ", "), dashes = 2 ("--"), len = 7 ("example")
        // - end points to " 123"
        int pos;
        int dashes;
        int len;
        int end() const { return pos + dashes + len; }
        operator bool() const { return dashes != 0 || len != 0; }
    };
    NamePos parse_desc(const char* desc) const;

    // 0 = positional, 1 = short, 2 = long
    std::pair<const char*, int> skip_dashes(const char* desc) const;

private:
    const char* m_desc;
    const char* m_help;
    Setter m_setter;
    enum Flags {
        FShort      = (1 << 0),
        FLong       = (1 << 1),
        FPositional = (1 << 2),
        FPrintHelp  = (1 << 3),
        FDots       = (1 << 3),
    };
    int m_flags = 0;
    int m_args = 0;
    int m_received = 0;
};


// Declarative parser for command-line arguments.

class ArgParser {
public:
    explicit ArgParser(std::initializer_list<Option> options);

    /// Parse command-line arguments
    ArgParser& parse_args(int argc, const char* argv[]);
    ArgParser& parse_args(int argc, char* argv[]) { return parse_args(argc, (const char**) argv); }

    // ------------------------------------------------------------------------
    // Semi-private methods
    // (normally not needed in client code)

    /// Add option to the parser
    ArgParser& add_option(Option&& opt);

    /// Print short usage information
    void print_usage() const;

    /// Print help text
    void print_help() const;

    /// Validate correctness of entered options
    void validate() const;

private:
    /// Parse environment variables
    /// This is called internally before `parse_args` (env has lower priority)
    void parse_env();

    /// Parse a single argument
    void parse_arg(const char* arg);

private:
    std::string m_progname;
    std::vector<Option> m_opts;
    Option* m_curopt = nullptr;
};


} // namespace xci::core::argparser

namespace xci::core { using argparser::ArgParser; }

#endif // include guard
