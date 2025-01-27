// demo_obj_parser.cpp created on 2023-11-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/scene/ObjParser.h>
#include <xci/core/ArgParser.h>

using namespace xci;
using namespace xci::core::argparser;

int main(int argc, const char* argv[])
{
    fs::path obj_file;
    ArgParser {
        Option("FILE", "Load this .obj file", obj_file),
    } (argv);

    bool ok = true;
    ObjParser obj_parser;
    if (!obj_file.empty()) {
        ok = obj_parser.parse_file(obj_file);
    }

    return !ok;
}
