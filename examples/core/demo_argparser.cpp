// demo_argparser.cpp created on 2020-01-28 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/core/ArgParser.h>
#include <iostream>

using namespace xci::core;
using namespace xci::core::argparser;
using std::cout;
using std::endl;

bool check_color(const char* arg)
{
    const std::string color(arg);
    return color == "red" || color == "green" || color == "blue";
}

int main(int argc, const char* argv[])
{
    bool verbose = false;
    int optimize = -1;
    const char* color = nullptr;
    std::vector<const char*> files;
    std::vector<const char*> rest;
    const char* pattern = nullptr;
    ArgParser {

        // short and long options - these are always optional (can't be required)
        Option("-h, --help", "Show help", show_help),
        Option("-v, --verbose", "Enable verbosity", verbose).env("VERBOSE"),
        Option("-O, --optimize LEVEL", "Optimization level", optimize),
        Option("-c, --color COLOR", "Choose color: red | green | blue",
                [&color](const char* arg){ color = arg; return check_color(arg); }),

        // positional arguments are required by default, surround them in "[]" to make them optional
        Option("PATTERN", "Required positional", pattern),
        Option("[FILE...]", "Input files", files),

        // special option to gather remaining arguments - this will trigger anytime
        // when encountered unknown argument or explicitly with delimiter arg: "--"
        // (always optional, brackets not needed)
        Option("-- ...", "Gather remaining arguments", rest),

    } (argv);

    cout << "OK: verbose=" << std::boolalpha << verbose << ", optimize=" << optimize << endl;
    cout << "    color: " << (color ? color : "[not given]") << endl;
    cout << "    pattern: " << (pattern ? pattern : "[not given]") << endl;
    cout << "    files:";
    for (const auto& f : files)
        cout << ' ' << f << ';';
    if (files.empty())
        cout << " [not given]";
    cout << endl;
    cout << "    passthrough:";
    for (const auto& r : rest)
        cout << ' ' << r << ';';
    if (rest.empty())
        cout << " [not given]";
    cout << endl;
    return 0;
}
