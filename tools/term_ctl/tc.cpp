// tc.cpp created on 2022-10-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

/// Term Ctl (tc) command line tool
/// Emits terminal control sequences

#include <xci/core/TermCtl.h>
#include <xci/core/ArgParser.h>

using namespace xci::core;
using namespace xci::core::argparser;


static constexpr auto c_version = "0.1";


int main(int argc, const char* argv[])
{
    bool show_version = false;
    bool isatty_always = false;
    std::vector<unsigned> tabs;

    ArgParser {
            Option("-t, --tabs N ...", "Set tab stops every N columns or N columns apart (when given multiple arguments)", tabs),
            Option("-f, --force", "Do not check isatty, always output the escape sequences", isatty_always),
            Option("-V, --version", "Show version", show_version),
            Option("-h, --help", "Show help", show_help),
    } (argv);

    TermCtl term(STDOUT_FILENO, isatty_always ? TermCtl::IsTty::Always : TermCtl::IsTty::Auto);

    if (show_version) {
        term.print("{t:bold}tc{t:normal} {}\n", c_version);
        return 0;
    }

    if (!tabs.empty()) {
        if (tabs.size() == 1)
            term.write(term.tab_set_every(tabs.front()).seq());
        else
            term.write(term.tab_set_all(tabs).seq());

        return 0;
    }

    auto size = term.size();
    term.print("size = {} cols, {} rows\n", size.cols, size.rows);
    return 0;
}
