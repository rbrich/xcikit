// ArgParser.h created on 2019-06-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020, 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_ARG_PARSER_H
#define XCI_CORE_ARG_PARSER_H

#include <xci/core/error.h>
#include <xci/compat/unistd.h>
#include <string>
#include <utility>
#include <vector>
#include <functional>
#include <sstream>
#include <initializer_list>
#include <iostream>
#include <cstdlib>
#include <limits>

namespace xci::core::argparser {


class BadOptionDescription: public xci::core::Error {
public:
    explicit BadOptionDescription(const std::string& detail, std::string_view desc)
            : Error(detail + ": " + std::string(desc)) {}
};

class BadArgument: public xci::core::Error {
public:
    explicit BadArgument(std::string detail)
            : Error(std::move(detail)) {}
};


// integers
template <class T>
typename std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, bool>
value_from_cstr(const char* s, T& value)
{
    char* end;
    errno = 0;
    long l = strtol(s, &end, 0);
    if (*end != '\0')
        return false;  // not fully parser
    if (errno == ERANGE || l > std::numeric_limits<T>::max() || l < std::numeric_limits<T>::min())
        return false;  // out of range
    value = static_cast<T>(l);
    return true;
}

// bool
template <class T>
typename std::enable_if_t<std::is_same_v<T, bool>, bool>
value_from_cstr(const char* s, T& value) {
    if ((*s == '0' || tolower(*s) == 'f' || tolower(*s) == 'n') && s[1] == '\0')
        value = false;
    else if ((*s == '1' || tolower(*s) == 't' || tolower(*s) == 'y') && s[1] == '\0')
        value = true;
    else if (!strcasecmp(s, "false") || !strcasecmp(s, "no"))
        value = false;
    else if (!strcasecmp(s, "true") || !strcasecmp(s, "yes"))
        value = true;
    else
        return false;
    return true;
}

// const char* (original, unparsed C string)
template <class T>
typename std::enable_if_t<
            std::is_same_v<T, const char*> || (
                std::is_assignable_v<T&, const char*> &&            // types with `operator=(const char*)`
                !std::is_trivially_assignable_v<T&, const char*>    // exclude `bool& v = const char* u` etc.
            ),
        bool>
value_from_cstr(const char* s, T& value) {
    value = s;
    return true;
}


// vector of unparsed args
template <class T>
bool value_from_cstr(const char* s, std::vector<T>& value) {
    T v;
    if (!value_from_cstr(s, v))
        return false;
    value.push_back(std::move(v));
    return true;
}


struct ShowHelp {};
constexpr ShowHelp show_help{};

struct Option {
    /// \param arg      current argument, or part of it, to be processed
    /// \return         true if the arg was accepted, false if rejected
    using Callback = std::function<bool(const char* arg)>;

    // simplified callbacks for special cases (process simple flag, process single arg)
    using FlagCallback = std::function<void()>;
    using ArgCallback = std::function<bool(const char* arg)>;

    /// This is the real, non-convenient constructor. See other constructors
    /// for some more convenience (simpler callbacks, direct binding of values etc.)
    ///
    /// \param desc     Option name(s) and arguments, e.g. "-o, --optimize LEVEL".
    ///                 The description is parsed - see tests for supported content.
    ///                 When printing usage and help, the description is colorized,
    ///                 but otherwise printed (mostly) as is specified.
    ///
    /// \param help     The text to show in help.
    ///
    /// \param cb       Callback is called whenever the option is encountered
    ///                 in arguments (argv). It's supposed to set a flag or variable
    ///                 or append the value to a list. It should not do any actual
    ///                 work, because it's called while still processing the arguments
    ///                 and there might be an error in later options.
    ///
    /// \param flags    The only valid flag here is FShowHelp. Other flags are internal
    ///                 and should not be passed through constructor. They are set
    ///                 automatically according to given description (`desc`).
    Option(std::string&& desc, std::string&& help, Callback cb, int flags);

    /// Declare option to show help and exit.
    Option(std::string desc, std::string help, ShowHelp)
            : Option(std::move(desc), std::move(help), nullptr, FShowHelp) {}

    /// Declare option to set a custom flag via callback.
    /// `desc` should describe a simple flag, e.g.: "-f, --flag"
    Option(std::string desc, std::string help, FlagCallback flag_cb)
            : Option(std::move(desc), std::move(help), [cb = std::move(flag_cb)](const char*)
                     { cb(); return true; }, 0) {}

    /// Declare option to set a custom value via callback.
    /// The callback can limit the accepted values by returning false (not accepted).
    /// `desc` should describe an option with value, e.g.: "-o, --output FILE"
    Option(std::string desc, std::string help, ArgCallback arg_cb)
            : Option(std::move(desc), std::move(help), [cb = std::move(arg_cb)](const char* arg)
                     { return cb(arg); }, 0) {}

    /// Declare option to set value of given variable.
    /// `desc` can describe either a flag or an option with value.
    /// See `value_from_cstr` templates above for accepted types
    /// and how they parse the value. E.g. for bool type,
    /// many usual values are recognized: 1, 0, false, true, y, n, ...
    template <class T>
    Option(std::string desc, std::string help, T& value)
            : Option(std::move(desc), std::move(help), [&value](const char* arg)
                     { return value_from_cstr(arg, value); }, 0) {}

    /// Attach env variable to this option
    Option& env(const char* env) { m_env = env; return *this; }

    bool has_short(char arg) const;
    bool has_long(const char* arg) const;
    bool has_args() const { return m_args != 0; }
    bool has_env(std::string_view env) const { return m_env && env == m_env; }
    bool is_short() const { return m_flags & FShort; }
    bool is_long() const { return m_flags & FLong; }
    bool is_positional() const { return m_flags & FPositional; }
    bool is_remainder() const { return m_flags & FRemainder; }
    bool is_show_help() const { return m_flags & FShowHelp; }
    bool can_receive_all_args() const { return (m_flags & FDots); }
    bool can_receive_arg() const { return can_receive_all_args() || m_received < m_args; }
    int required_args() const { return m_required; }
    int missing_args() const;
    const std::string& desc() const { return m_desc; }
    const std::string& help() const { return m_help; }
    const char* env() const { return m_env; }
    std::string usage() const;
    std::string formatted_desc(size_t width) const;

    /// Call `cb` for each declared name (either shortopt or longopt will be passed,
    /// never both - missing one will be 0/empty)
    void foreach_name(const std::function<void(char shortopt, std::string_view longopt)>& cb) const;

    bool operator() (const char* arg) { ++m_received; return m_cb(arg); }
    void eval_env(const char* env) { m_cb(env); }

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
    static NamePos parse_desc(const char* desc) ;

    // 0 = positional, 1 = short, 2 = long
    static std::pair<const char*, int> skip_dashes(const char* desc) ;

private:
    std::string m_desc;
    std::string m_help;
    const char* m_env = nullptr;
    Callback m_cb;
    enum Flags {
        FShort      = (1 << 0),
        FLong       = (1 << 1),
        FPositional = (1 << 2),
        FShowHelp   = (1 << 3),
        FDots       = (1 << 4),
        FRemainder  = (1 << 5),
    };
    int m_flags = 0;
    int m_args = 0;     // number of expected args (infinite with FDots)
    int m_required = 0; // number of required args
    int m_received = 0; // number of actually received args
};


/// Declarative parser for command-line arguments.
///
/// References:
/// - https://www.gnu.org/software/libc/manual/html_node/Argument-Syntax.html
/// - https://github.com/CLIUtils/CLI11 (there are some similarities in concept)
///
/// TODO:
/// - conform to POSIX
/// - support '=' between option and its argument
/// - "--" should terminate option args, but otherwise not change parsing
///   behaviour (i.e. the args should still be parsed as normal positional
///   arguments, instead of only special remainder args)

class ArgParser {
public:
    explicit ArgParser(std::initializer_list<Option> options);

    /// Add option to the parser (prefer passing all options to constructor)
    ArgParser& add_option(Option&& opt);

    /// Validate correctness of entered options (e.g. doubled flags)
    void validate() const;

    /// Main - process env variables and command-line arguments,
    /// exit on error or after showing help.
    /// This methods expects complete set of arguments in one go.
    /// It will fail on missing arguments of trailing options.
    /// For incremental parsing, see `parse_args` below.
    ///
    /// \param argv     argv argument from main() function, i.e. program name,
    ///                 followed by args, followed by NULL terminator
    ArgParser& operator()(const char* argv[]);
    ArgParser& operator()(char* argv[]) { return operator()((const char**) argv); }

    enum ParseResult {
        Continue,
        Stop,
        Exit,
    };

    /// Parse argv[0] which contains full name (path) to this program.
    /// The basename is remembered and used in help message.
    bool parse_program_name(const char* arg0);

    /// Lookup for declared options in `environ` (see Option::env).
    /// This is called internally before `parse_args` (env has lower priority)
    void parse_env();

    /// Incrementally parse command-line arguments, throw on errors
    /// \param argv     NULL-terminated arguments, without main's argv[0]
    ///                 (the program name)
    /// \param finish   true = expect all (or the rest) of args given to this call,
    ///                 false = partial parse, do not throw if the last option
    ///                 requires an argument (but there is no more args to parse).
    ParseResult parse_args(const char* argv[], bool finish = true);

    /// Parse a single argument
    ParseResult parse_arg(const char* argv[]);

    /// Print short usage information
    /// \param max_width    Wrap lines when reaching this number of columns. 0 = no wrapping
    void print_usage(unsigned max_width = 100) const;

    /// Print help text
    /// \param max_width    Wrap lines when reaching this number of columns. 0 = no wrapping
    void print_help(unsigned max_width = 100) const;

    /// Print information how to invoke help
    void print_help_notice() const;

private:
    bool invoke_remainder(const char** argv);

    std::string m_progname;
    std::vector<Option> m_opts;
    Option* m_curopt = nullptr;
    bool m_awaiting_arg = false;
};


} // namespace xci::core::argparser

namespace xci::core { using argparser::ArgParser; }

#endif // include guard
