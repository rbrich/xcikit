// demo_termctl.cpp created on 2018-07-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/core/TermCtl.h>
#include <xci/core/string.h>

#include <magic_enum/magic_enum.hpp>

#include <iostream>
#include <iomanip>
#include <cwctype>

using namespace xci::core;
using std::cout;
using std::endl;

int main()
{
    TermCtl& t = TermCtl::stdout_instance();

    cout << (t.is_tty() ? "terminal initialized" : "terminal not supported") << endl;
    cout << "size: " << t.size() << endl;
    cout << t.bold().red().on_blue() << "SUPER RED " << t.normal() << "<-----" << endl;

    cout << t.move_up().move_right(6).bold().green() << "GREEN" <<t.normal() << endl;

    t.print("<b><yellow>formatted <*white><@yellow> bg <n>\n");
    t.print("<bold>bold<normal_intensity> "
            "<dim>dim<normal_intensity> "
            "<italic>italic<no_italic> "
            "<underline>underlined<no_underline> "
            "<overline>overlined<no_overline> "
            "<cross_out>crossed out<no_cross_out> "
            "<frame>framed<no_frame> "
            "<blink>blinking<no_blink> "
            "<reverse>reversed<no_reverse> "
            "<hidden>hidden<no_hidden> "
            "\n");
    t.print("Escaped \\<bold>. Unknown <tag>.\n");

    t.tab_set_all({30, 20}).write();
    t.print("tab stops:\t1\t2\n");
    t.tab_set_every(8).write();

    TermCtl& tin = TermCtl::stdin_instance();
    bool done = false;
    while (!done) {
        const std::string in = tin.raw_input();
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
            switch (towupper(wint_t(decoded.unicode))) {
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
