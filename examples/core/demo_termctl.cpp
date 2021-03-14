// demo_termctl.cpp created on 2018-07-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/core/TermCtl.h>
#include <xci/core/string.h>

#include <magic_enum.hpp>

#include <iostream>
#include <iomanip>
#include <cwctype>

using namespace std;
using namespace xci::core;

int main()
{
    TermCtl& t = TermCtl::stdout_instance();

    cout << (t.is_tty() ? "terminal initialized" : "terminal not supported") << endl;
    cout << t.bold().red().on_blue() << "SUPER RED " << t.normal() << "<-----" << endl;

    cout << t.move_up().move_right(6).bold().green() << "GREEN" <<t.normal() << endl;

    TermCtl& tin = TermCtl::stdin_instance();
    bool done = false;
    while (!done) {
        std::string in = tin.raw_input(false);
        cout << "\nKey pressed:\n";

        cout << "* seq: ";
        for (const auto c : in) {
            cout << std::hex << std::setw(2) << std::setfill('0')
                 << (int) (unsigned char) c << " ";
        }
        cout << '"' << escape(in, true) << '"' << "\n";

        auto decoded = tin.decode_input(in);
        cout << "* decoded: " << decoded.input_len << " bytes\n";
        if (decoded.key != TermCtl::Key::UnicodeChar)
            cout << "* key: " << magic_enum::enum_name(decoded.key) << "\n";
        if (decoded.mod)
            cout << "* modifiers: " << decoded.mod << "\n";
        if (decoded.unicode != 0)
            cout << "* unicode: " << uint32_t(decoded.unicode)
                 << " '" << to_utf8(decoded.unicode) << "'\n";
        // handle Ctrl-C
        if (decoded.mod.is_ctrl() && decoded.key == TermCtl::Key::UnicodeChar) {
            switch (towupper(decoded.unicode)) {
                case 'C':
                case 'D':
                case 'Z':
                    done = true;
                    break;
            }
        }
    }
    return 0;
}
