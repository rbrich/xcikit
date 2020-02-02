// ArgParser.cpp created on 2019-06-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ArgParser.h"
#include "file.h"
#include "format.h"
#include "TermCtl.h"
#include <iostream>
#include <utility>
#include <cstring>
#include <cassert>

namespace xci::core::argparser {

using namespace std;


Option::Option(const char* desc, const char* help, Callback cb, int flags)
    : m_desc(desc), m_help(help), m_cb(move(cb)), m_flags(flags)
{
    for (;;) {
        auto p = parse_desc(desc);
        if (!p)
            break;

        // sanity check
        if ((m_flags & FPositional) && p.dashes > 0)
            throw BadOptionDescription("short/long option after positional name", desc + p.pos);

        if (p.dashes > 2)
            throw BadOptionDescription("too many dashes", desc + p.pos);
        else if (p.dashes == 2) {
            if (p.len == 0) {
                m_flags |= FStop;
            } else
                m_flags |= FLong;
        } else if (p.dashes == 1) {
            if (p.len != 1)
                throw BadOptionDescription("short option must contain a single character", desc + p.pos);
            m_flags |= FShort;
        } else {
            assert(p.dashes == 0);
            if (desc[p.pos] == '.') {
                if (p.len != 3)
                    throw BadOptionDescription("use three dots for ellipsis", desc + p.pos);
                m_flags |= FDots;
            } else if ((m_flags & (FShort | FLong)) == 0) {
                assert(!m_args);
                m_flags |= FPositional;
                m_args = 1;
            } else {
                ++ m_args;
            }
        }
        desc += p.end();
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


bool Option::has_short(char arg) const
{
    if (!is_short())
        return false;
    auto* desc = m_desc;
    for (;;) {
        auto p = parse_desc(desc);
        if (!p.len)
            break;
        if (p.dashes == 1 && desc[p.pos + p.dashes] == arg)
            return true;
        desc += p.end();
    }
    return false;
}


bool Option::has_long(const char* arg) const
{
    if (!is_long())
        return false;
    auto* desc = m_desc;
    for (;;) {
        auto p = parse_desc(desc);
        if (!p.len)
            break;
        if (p.dashes == 2 && !strncmp(desc + p.pos + p.dashes, arg, p.len))
            return true;
        desc += p.end();
    }
    return false;
}


std::string Option::usage() const
{
    auto& t = TermCtl::stdout_instance();
    string res;
    auto* desc = m_desc;
    for (;;) {
        auto p = parse_desc(desc);
        if (!p)
            break;
        if (res.empty())
            res = t.format("{bold}{green}{}{normal}", std::string(desc + p.pos, p.dashes + p.len));
        else if (!p.dashes)
            res += t.format(" {green}{}{normal}", std::string(desc + p.pos, p.len));
        desc += p.end();
    }
    return res;
}


std::string Option::formatted_desc(size_t width) const
{
    auto& t = TermCtl::stdout_instance();
    string res = m_desc;
    size_t res_pos = 0;
    auto* desc = m_desc;
    for (;;) {
        auto p = parse_desc(desc);
        if (!p)
            break;
        res_pos += p.pos;

        const auto color = (p.dashes || ((m_flags & FPositional) && desc[p.pos] != '.'))
                     ? t.bold().green().seq()
                     : t.green().seq();
        res.insert(res_pos, color);
        res_pos += color.length() + p.dashes + p.len;

        const auto normal = t.normal().seq();
        res.insert(res_pos, normal);
        res_pos += normal.length();

        desc += p.end();
    }
    return res + std::string(width - strlen(m_desc), ' ');
}


ArgParser::ArgParser(initializer_list<Option> options)
    : m_opts(options)
{
    validate();
}


void ArgParser::parse_env()
{
    // TODO
}


ArgParser::ParseResult ArgParser::parse_args(int argc, const char** argv)
{
    if (argc < 1)
        throw BadArgument("missing program name (argv[0])");
    m_progname = path_basename(*argv);
    while (--argc) {
        switch (parse_arg(++argv)) {
            case Continue:  continue;
            case Stop:      return Stop;
            case Exit:      return Exit;
        }
    }
    return Continue;
}


ArgParser& ArgParser::add_option(Option&& opt)
{
    m_opts.push_back(move(opt));
    return *this;
}


void ArgParser::print_usage() const
{
    auto& t = TermCtl::stdout_instance();
    cout << t.format("Usage: {bold}{}{normal} ", m_progname);
    for (const auto& opt : m_opts) {
        cout << '[' << opt.usage() << "] ";
    }
    cout << endl;
}


void ArgParser::print_help() const
{
    size_t desc_cols = 0;
    for (const auto& opt : m_opts)
        desc_cols = max(desc_cols, strlen(opt.desc()));
    print_usage();
    cout << endl << "Options:" << endl;
    for (const auto& opt : m_opts) {
        cout << "  " << opt.formatted_desc(desc_cols)
             << "  " << opt.help() << endl;
    }
}


void ArgParser::validate() const
{
    // TODO
}


ArgParser::ParseResult ArgParser::parse_arg(const char* argv[])
{
    int dashes = 0;  // 0 = positional, 1 = short, 2 = long
    const char* arg = *argv;
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
            auto it = find_if(m_opts.begin(), m_opts.end(),
                    [](const Option& opt) { return opt.is_stop(); });
            if (it == m_opts.end())
                throw BadArgument(format("Unknown option: {}", arg));
            (*it)(argv + 1);
            return Stop;
        }
        // long option
        auto it = find_if(m_opts.begin(), m_opts.end(),
                [p](const Option& opt) { return opt.has_long(p); });
        if (it == m_opts.end())
            throw BadArgument(format("Unknown option: {}", arg));
        if (!it->has_args()) {
            if (it->is_print_help()) {
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
        return Continue;
    }
    if (dashes == 1) {
        // short option
        while (*p) {
            auto it = find_if(m_opts.begin(), m_opts.end(),
                    [p](const Option& opt) { return opt.has_short(*p); });
            if (it == m_opts.end())
                throw BadArgument(format("Unknown option: -{} (in {})", p, arg));
            ++p;
            if (!it->has_args()) {
                if (it->is_print_help()) {
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
            if (*p) {
                if (! (*it)(p) )
                    throw BadArgument(format("Wrong value to option: {}: {}", p[-1], p));
            }
            m_curopt = &*it;
            break;
        }
        return Continue;
    }
    assert(dashes == 0);
    if (m_curopt && m_curopt->can_receive_arg()) {
        // open option -> pass it the arg
        (*m_curopt)(arg);
    } else {
        auto it = find_if(m_opts.begin(), m_opts.end(),
                [](const Option& opt) { return opt.is_positional() && opt.can_receive_arg(); });
        if (it == m_opts.end()) {
            auto it_stop = find_if(m_opts.begin(), m_opts.end(),
                    [](const Option& opt) { return opt.is_stop(); });
            if (it_stop == m_opts.end())
                throw BadArgument(format("Unexpected positional argument: {}", arg));
            (*it_stop)(argv);
            return Stop;
        } if (! (*it)(p) )
            throw BadArgument(format("Wrong positional argument: {}", p));
    }
    return Continue;
}


ArgParser& ArgParser::operator()(int argc, const char* argv[])
{
    try {
        switch (parse_args(argc, argv)) {
            case Exit:
                exit(0);
            case Continue:
            case Stop:
                break;
        }
    } catch (const BadArgument& e) {
        cerr << e.what() << endl;
        exit(1);
    }
    return *this;
}


} // namespace xci::core::argparser
