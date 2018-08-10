// test_widgets.cpp created on 2018-08-06, part of XCI toolkit
// Copyright 2018 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <xci/widgets/TextTerminal.h>
#include <xci/util/string.h>
#include <xci/util/format.h>

using namespace xci::widgets;
using namespace xci::util;


TEST_CASE( "Attributes", "[TextTerminal]" )
{
    terminal::Attributes attr;
    terminal::Attributes attr2;
    std::string enc;

    CHECK( attr.encode().empty() );

    attr.set_fg(7);
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}\x07", terminal::c_ctl_fg8bit)) );
    CHECK( attr2.decode(enc) == 2 );
    CHECK( escape(attr2.encode()) == escape(enc) );

    attr.set_bg(15);
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}\x07{}\x0f", terminal::c_ctl_fg8bit, terminal::c_ctl_bg8bit)) );
    CHECK( attr2.decode(enc) == 4 );
    CHECK( escape(attr2.encode()) == escape(enc) );

    attr.set_fg(terminal::Color24bit(0x40, 0x50, 0x60));
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}\x0f{}\x40\x50\x60", terminal::c_ctl_bg8bit, terminal::c_ctl_fg24bit)) );
    CHECK( attr2.decode(enc) == 6 );
    CHECK( escape(attr2.encode()) == escape(enc) );

    attr.set_bg(terminal::Color24bit(0x70, 0x80, 0x90));
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}\x40\x50\x60{}\x70\x80\x90", terminal::c_ctl_fg24bit, terminal::c_ctl_bg24bit)) );

    attr.reset_fg();
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}{}\x70\x80\x90", terminal::c_ctl_default_fg, terminal::c_ctl_bg24bit)) );

    attr.reset_bg();
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}{}", terminal::c_ctl_default_fg, terminal::c_ctl_default_bg)) );

    attr.set_bold(true);
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}\x02{}{}", terminal::c_ctl_set_attrs, terminal::c_ctl_default_fg, terminal::c_ctl_default_bg)) );

    attr.set_italic(false);
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}\x02{}\x01{}{}", terminal::c_ctl_set_attrs, terminal::c_ctl_reset_attrs, terminal::c_ctl_default_fg, terminal::c_ctl_default_bg)) );
}
