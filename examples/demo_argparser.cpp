// demo_argparser.cpp created on 2020-01-28 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/core/ArgParser.h>
#include <iostream>

using namespace xci::core;
using namespace xci::core::argparser;
using namespace std;

int main(int argc, const char* argv[])
{
    bool verbose = false;
    int optimize = -1;
    std::vector<std::string> files;
    ArgParser {

        Option("-h, --help", "Show help", show_help),
        Option("-v, --verbose", "Enable verbosity", verbose),
        Option("-O, --optimize LEVEL", "Optimization level", optimize),
        Option("FILE...", "Input files", [&files](std::string_view arg){ files.emplace_back(arg); return true; }),

    } .parse_args(argc, argv);

    cout << "OK: verbose=" << boolalpha << verbose << ", optimize=" << optimize << endl;
    cout << "  files:";
    for (const auto& f : files)
        cout << ' ' << f << ';';
    cout << endl;
    return 0;
}
