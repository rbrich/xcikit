// ArgParser.cpp created on 2019-06-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ArgParser.h"
#include "file.h"
#include "format.h"
#include "TermCtl.h"
#include "string.h"
#include <iostream>
#include <utility>
#include <cstring>
#include <cassert>
#include <algorithm>

namespace xci::core::argparser {

using namespace std;


Option::Option(std::string desc, const char* help, Callback cb, int flags)
    : m_desc(move(desc)), m_help(help), m_cb(move(cb)), m_flags(flags)
{
    // check and remove brackets from both ends
    strip(m_desc, ' ');
    bool optional = (m_desc[0] == '[');
    lstrip(m_desc, '[');
    rstrip(m_desc, ']');

    auto* dp = m_desc.c_str();
    for (;;) {
        auto p = parse_desc(dp);
        if (!p)
            break;

        // sanity check
        if ((m_flags & FPositional) && p.dashes > 0)
            throw BadOptionDescription("short/long option after positional name", dp + p.pos);

        if (p.dashes > 2)
            throw BadOptionDescription("too many dashes", dp + p.pos);
        else if (p.dashes == 2) {
            if (p.len == 0) {
                m_flags |= FRemainder;
            } else
                m_flags |= FLong;
        } else if (p.dashes == 1) {
            if (p.len != 1)
                throw BadOptionDescription("short option must contain a single character", dp + p.pos);
            m_flags |= FShort;
        } else {
            assert(p.dashes == 0);
            if (dp[p.pos] == '.') {
                if (p.len != 3)
                    throw BadOptionDescription("use three dots for ellipsis", dp + p.pos);
                m_flags |= FDots;
            } else if ((m_flags & (FShort | FLong)) == 0) {
                assert(!m_args);
                m_flags |= FPositional;
                m_args = 1;
            } else {
                ++ m_args;
            }
        }
        dp += p.end();
    }

    if (optional && !is_positional())
        throw BadOptionDescription("non-positional argument can't be made optional", m_desc);
    m_required = optional ? 0 : m_args;
}


bool Option::has_short(char arg) const
{
    if (!is_short())
        return false;
    auto* dp = m_desc.c_str();
    for (;;) {
        auto p = parse_desc(dp);
        if (!p.len)
            break;
        if (p.dashes == 1 && dp[p.pos + p.dashes] == arg)
            return true;
        dp += p.end();
    }
    return false;
}


bool Option::has_long(const char* arg) const
{
    if (!is_long())
        return false;
    auto* dp = m_desc.c_str();
    for (;;) {
        auto p = parse_desc(dp);
        if (!p.len)
            break;
        if (p.dashes == 2 && !strncmp(dp + p.pos + p.dashes, arg, p.len))
            return true;
        dp += p.end();
    }
    return false;
}


int Option::missing_args() const
{
    return max(m_required - m_received, 0);
}


std::string Option::usage() const
{
    auto& t = TermCtl::stdout_instance();
    string res;
    bool required = is_positional() && required_args() != 0;
    if (!required)
        res += '[';
    bool first = true;
    auto* dp = m_desc.c_str();
    for (;;) {
        auto p = parse_desc(dp);
        if (!p)
            break;
        if (first) {
            first = false;
            res += t.format("{bold}{green}{}{normal}", std::string(dp + p.pos, p.dashes + p.len));
        } else if (!p.dashes) {
            res += t.format(" {green}{}{normal}", std::string(dp + p.pos, p.len));
        }
        dp += p.end();
    }
    if (!required)
        res += ']';
    return res;
}


std::string Option::formatted_desc(size_t width) const
{
    auto& t = TermCtl::stdout_instance();
    string res(m_desc);
    size_t res_pos = 0;
    auto* dp = m_desc.c_str();
    for (;;) {
        auto p = parse_desc(dp);
        if (!p)
            break;
        res_pos += p.pos;

        const auto color = (p.dashes || ((m_flags & FPositional) && dp[p.pos] != '.'))
                     ? t.bold().green().seq()
                     : t.green().seq();
        res.insert(res_pos, color);
        res_pos += color.length() + p.dashes + p.len;

        const auto normal = t.normal().seq();
        res.insert(res_pos, normal);
        res_pos += normal.length();

        dp += p.end();
    }
    return res + std::string(width - m_desc.size(), ' ');
}


void Option::foreach_name(const std::function<void(char shortopt, std::string_view longopt)>& cb) const
{
    auto* dp = m_desc.c_str();
    for (;;) {
        auto p = parse_desc(dp);
        if (!p)
            break;
        auto* name = dp + p.pos + p.dashes;
        if (p.dashes == 1 && p.len == 1)
            cb(name[0], {});
        if (p.dashes == 2 && p.len > 0)
            cb(0, std::string_view(name, p.len));
        dp += p.end();
    }
}


Option::NamePos Option::parse_desc(const char* desc) const
{
    // skip spaces and commas
    int pos = 0;
    while (*desc == ',' || *desc == ' ') {
        ++desc;
        ++pos;
    }

    // handle ellipsis as special kind of name
    int len = 0;
    while (*desc == '.') {
        ++desc;
        ++len;
    }
    if (len > 0)
        return {pos, 0, len};

    // count dashes
    auto p = skip_dashes(desc);
    desc = p.first;

    // keyword
    while (*desc && *desc != ',' && *desc != ' ' && *desc != '.') {
        ++desc;
        ++len;
    }
    return {pos, p.second, len};
}


std::pair<const char*, int> Option::skip_dashes(const char* desc) const
{
    int dashes = 0;
    while (*desc == '-') {
        ++desc;
        ++dashes;
    }
    return {desc, dashes};
}


// ----------------------------------------------------------------------------


ArgParser::ArgParser(initializer_list<Option> options)
    : m_opts(options)
{
    validate();
}


ArgParser& ArgParser::add_option(Option&& opt)
{
    m_opts.push_back(move(opt));
    return *this;
}


void ArgParser::validate() const
{
    std::vector<char> shorts;
    std::vector<string_view> longs;
    std::vector<const char*> envs;
    for (const auto& opt : m_opts) {
        // check short, long
        opt.foreach_name([&opt, &shorts, &longs](char shortopt, string_view longopt) {
            if (shortopt != 0) {
                if (std::find_if(shorts.cbegin(), shorts.cend(),
                        [shortopt](char v){ return shortopt == v; }) != shorts.cend())
                    throw BadOptionDescription(format("name -{} repeated", shortopt), opt.desc());
                shorts.push_back(shortopt);
            } else {
                assert(!longopt.empty());
                if (std::find_if(longs.cbegin(), longs.cend(),
                        [longopt](string_view v){ return longopt == v; }) != longs.cend())
                    throw BadOptionDescription(format("name --{} repeated", longopt), opt.desc());
                longs.push_back(longopt);
            }
        });
        // check env
        if (opt.env()) {
            if (std::find_if(envs.cbegin(), envs.cend(),
                    [&opt](const char* v){ return !strcmp(v, opt.env()); }) != envs.cend())
                throw BadOptionDescription("env name repeated", opt.env());
            envs.push_back(opt.env());
        }
    }
}


ArgParser& ArgParser::operator()(const char* argv[])
{
    if (!parse_program_name(argv[0])) {
        // this should not occur
        auto& t = TermCtl::stderr_instance();
        cerr << t.bold().red() << "Missing program name (argv[0])" << t.normal() << endl;
        exit(1);
    }
    try {
        parse_env();
        switch (parse_args(argv + 1, true)) {
            case Exit:
                exit(0);
            case Continue:
            case Stop:
                break;
        }
    } catch (const BadArgument& e) {
        auto& t = TermCtl::stderr_instance();
        cerr << t.bold().red() << e.what() << t.normal() << endl;
        print_usage();
        exit(1);
    }
    return *this;
}


bool ArgParser::parse_program_name(const char* arg0)
{
    if (!arg0)
        return false;
    m_progname = path::basename(arg0);
    return true;
}


void ArgParser::parse_env()
{
    char** env = environ;
    while (*env) {
        auto s = split(*env, '=', 1);
        if (s.size() != 2)
            continue;
        auto key = s[0];
        auto val = s[1];
        auto it = find_if(m_opts.begin(), m_opts.end(),
                [&key](const Option& opt) { return opt.has_env(key); });
        if (it != m_opts.end()) {
            assert(!it->is_show_help());  // help can't be invoked via env
            it->eval_env(val.data());
        }
        ++env;
    }
}


ArgParser::ParseResult ArgParser::parse_args(const char** argv, bool finish)
{
    while (*argv) {
        switch (parse_arg(argv++)) {
            case Continue:  continue;
            case Stop:      return Stop;
            case Exit:      return Exit;
        }
    }
    if (finish) {
        if (m_awaiting_arg)
            throw BadArgument(format("Missing value to option: {}", *(argv-1)));
        for (const auto& opt : m_opts) {
            if (opt.is_positional() && opt.missing_args() > 0)
                throw BadArgument(format("Missing required arguments: {}", opt.desc()));
        }
    }
    return Continue;
}


ArgParser::ParseResult ArgParser::parse_arg(const char* argv[])
{
    int dashes = 0;  // 0 = positional, 1 = short, 2 = long
    const char* arg = *argv;

    if (m_awaiting_arg) {
        // open option -> pass it the arg
        assert(m_curopt->can_receive_arg());  // already checked
        if (!(*m_curopt)(arg))
            throw BadArgument(format("Wrong value to option: {}: {}", *(argv-1), arg));
        m_awaiting_arg = false;
        return Continue;
    }

    const char* p = arg;
    while (*p == '-') {
        ++p;
        ++dashes;
    }
    if (dashes > 2)
        throw BadArgument(format("Too many dashes: {}", arg));
    if (dashes == 2) {
        // stop parsing (--)
        if (*p == 0) {
            if (invoke_remainder(argv + 1))
                return Stop;
            throw BadArgument(format("Unknown option: {}", arg));
        }
        // long option
        auto it = find_if(m_opts.begin(), m_opts.end(),
                [p](const Option& opt) { return opt.has_long(p); });
        if (it == m_opts.end()) {
            if (invoke_remainder(argv))
                return Stop;
            throw BadArgument(format("Unknown option: {}", arg));
        }
        if (!it->has_args()) {
            if (it->is_show_help()) {
                print_help();
                return Exit;
            }
            if (! (*it)("1") )
                throw BadArgument(format("Wrong value to option: {}: 1", arg));
            return Continue;
        }
        if (!it->can_receive_arg())
            throw BadArgument(format("Too many occurrences of an option: {}", arg));
        m_curopt = &*it;
        m_awaiting_arg = true;
        return Continue;
    }
    if (dashes == 1 && *p) {
        // short option
        while (*p) {
            auto it = find_if(m_opts.begin(), m_opts.end(),
                    [p](const Option& opt) { return opt.has_short(*p); });
            if (it == m_opts.end()) {
                if (p == arg + 1 && invoke_remainder(argv))
                    return Stop;
                throw BadArgument(format("Unknown option: -{} (in {})", p, arg));
            }
            ++p;
            if (!it->has_args()) {
                if (it->is_show_help()) {
                    print_help();
                    return Exit;
                }
                if (! (*it)("1") )
                    throw BadArgument(format("Wrong value to option: {}: 1", arg));
                continue;
            }
            // has args
            if (!it->can_receive_arg())
                throw BadArgument(format("Too many occurrences of an option: -{} (in {})", p[-1], arg));
            m_curopt = &*it;
            if (*p) {
                if (! (*it)(p) )
                    throw BadArgument(format("Wrong value to option: {}: {}", p[-1], p));
                break;
            }
            m_awaiting_arg = true;
            break;
        }
        return Continue;
    }
    assert(dashes == 0 || (dashes == 1 && *p == 0));
    if (m_curopt && m_curopt->can_receive_arg()) {
        // open option -> pass it the arg
        (*m_curopt)(arg);
    } else {
        auto it = find_if(m_opts.begin(), m_opts.end(),
                [](const Option& opt) { return opt.is_positional() && opt.can_receive_arg(); });
        if (it == m_opts.end()) {
            if (invoke_remainder(argv))
                return Stop;
            throw BadArgument(format("Unexpected positional argument: {}", arg));
        } if (! (*it)(arg) )
            throw BadArgument(format("Wrong positional argument: {}", arg));
    }
    return Continue;
}


void ArgParser::print_usage() const
{
    auto& t = TermCtl::stdout_instance();
    cout << t.format("Usage: {bold}{}{normal} ", m_progname);
    for (const auto& opt : m_opts) {
        cout << opt.usage() << ' ';
    }
    cout << endl;
}


void ArgParser::print_help() const
{
    size_t desc_cols = 0;
    for (const auto& opt : m_opts)
        desc_cols = max(desc_cols, opt.desc().size());
    print_usage();
    cout << endl << "Options:" << endl;
    for (const auto& opt : m_opts) {
        cout << "  " << opt.formatted_desc(desc_cols)
             << "  " << opt.help() << endl;
    }
}


bool ArgParser::invoke_remainder(const char** argv)
{
    auto it = find_if(m_opts.begin(), m_opts.end(),
            [](const Option& opt) { return opt.is_remainder(); });
    if (it == m_opts.end())
        return false;
    return (*it)(argv);
}


} // namespace xci::core::argparser
