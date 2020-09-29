// demo_termctl.cpp created on 2018-07-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/core/TermCtl.h>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace xci::core;

int main()
{
    TermCtl& t = TermCtl::stdout_instance();

    cout << (t.is_tty() ? "terminal initialized" : "terminal not supported") << endl;
    cout << t.bold().red().on_blue() << "SUPER RED " << t.normal() << "<-----" << endl;

    cout << t.move_up().move_right(6).bold().green() << "GREEN" <<t.normal() << endl;

    std::string in = t.raw_input();
    cout << "Key pressed: ";
    for (const auto c : in)
        cout << std::hex << std::setw(2) << std::setfill('0')
             << (int) c << " ";
    cout << endl;
    return 0;
}
