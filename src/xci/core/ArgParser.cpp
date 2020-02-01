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


template <class... Ts>
static void die(Ts... args) {
    cerr << format(args...) << endl;
    exit(1);
}


Option::Option(const char* desc, const char* help, Setter setter, int flags)
    : m_desc(desc), m_help(help), m_setter(move(setter)), m_flags(flags)
{
    for (;;) {
        auto p = parse_desc(desc);
        if (!p)
            break;

        if (p.dashes >= 2) {
            assert(p.dashes == 2);
            m_flags |= FLong;
        } else if (p.dashes == 1) {
            assert(p.len == 1);
            m_flags |= FShort;
        } else {
            assert(p.dashes == 0);
            if (desc[p.pos] == '.') {
                assert(p.len == 3);
                m_flags |= FDots;
            } else if ((m_flags & (FShort | FLong)) == 0) {
                assert(!m_args);
                m_flags |= FPositional;
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


ArgParser& ArgParser::parse_args(int argc, const char** argv)
{
    if (argc > 0) {
        m_progname = path_basename(*argv);
    }
    while (--argc) {
        parse_arg(*++argv);
    }
    return *this;
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


void ArgParser::parse_arg(const char* arg)
{
    int dashes = 0;  // 0 = positional, 1 = short, 2 = long
    const char* p = arg;
    while (*p == '-') {
        ++p;
        ++dashes;
    }
    if (dashes > 2)
        die("Too many dashes: {}", arg);
    if (dashes == 2) {
        // long option
        auto it = find_if(m_opts.begin(), m_opts.end(),
                [p](const Option& opt) { return opt.has_long(p); });
        if (it == m_opts.end())
            die("Unknown option: {}", arg);
        if (!it->has_args()) {
            if (it->is_print_help()) {
                print_help();
                exit(0);
            }
            bool set_flag_success = (*it)("1");
            assert(set_flag_success);
            (void) set_flag_success;
            return;
        }
        m_curopt = &*it;
        return;
    }
    if (dashes == 1) {
        // short option
        while (*p) {
            auto it = find_if(m_opts.begin(), m_opts.end(),
                    [p](const Option& opt) { return opt.has_short(*p); });
            if (it == m_opts.end())
                die("Unknown option: -{} (in {})", p, arg);
            ++p;
            if (!it->has_args()) {
                if (it->is_print_help()) {
                    print_help();
                    exit(0);
                }
                bool set_flag_success = (*it)("1");
                assert(set_flag_success);
                (void) set_flag_success;
                continue;
            }
            // has args
            if (*p) {
                bool set_flag_success = (*it)(p);
                assert(set_flag_success);
                (void) set_flag_success;
            }
            m_curopt = &*it;
            break;
        }
        return;
    }
    assert(dashes == 0);
    if (m_curopt && m_curopt->can_receive()) {
        // open option -> pass it the arg
        (*m_curopt)(arg);
    } else {
        auto it = find_if(m_opts.begin(), m_opts.end(),
                [](const Option& opt) { return opt.is_positional() && opt.can_receive(); });
        if (it == m_opts.end())
            die("Unexpected positional argument: {}", arg);
        bool set_flag_success = (*it)(p);
        assert(set_flag_success);
        (void) set_flag_success;
        return;
    }
}


} // namespace xci::core::argparser
