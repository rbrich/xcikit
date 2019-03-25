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
#include <catch2/catch.hpp>

#include <xci/widgets/TextTerminal.h>
#include <xci/core/string.h>
#include <xci/core/format.h>

using namespace xci::widgets;
using namespace xci::text;
using namespace xci::graphics;
using namespace xci::core;
using namespace xci::widgets::terminal::ctl;

class TestRenderer: public terminal::Renderer {
public:
    void set_font_style(FontStyle font_style) override {
        if (font_style == m_font_style)
            return;
        m_output.append([font_style](){
            switch (font_style) {
                default:
                case FontStyle::Regular:    return "[r]";
                case FontStyle::Italic:     return "[i]";
                case FontStyle::Bold:       return "[b]";
                case FontStyle::BoldItalic: return "[bi]";
            }
        }());
        m_font_style = font_style;
    }
    void set_fg_color(Color fg) override {
        m_output.append(format("[fg:{}]"));
    }
    void set_bg_color(Color bg) override {
        m_output.append(format("[bg:{}]"));
    }
    void draw_blanks(size_t num) override {
        m_output.append(num, ' ');
    }
    void draw_char(CodePoint code_point) override {
        m_output.append(to_utf8(code_point));
    }

    std::string output() { return std::move(m_output); }

private:
    std::string m_output;
    FontStyle m_font_style = FontStyle::Regular;
};


TEST_CASE( "Attributes", "[TextTerminal]" )
{
    terminal::Attributes attr;
    terminal::Attributes attr2;
    std::string enc;

    CHECK( attr.encode().empty() );

    attr.set_fg(7);
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}\x07", fg8bit)) );
    CHECK( attr2.decode(enc) == 2 );
    CHECK( escape(attr2.encode()) == escape(enc) );

    attr.set_bg(15);
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}\x07{}\x0f", fg8bit, bg8bit)) );
    CHECK( attr2.decode(enc) == 4 );
    CHECK( escape(attr2.encode()) == escape(enc) );

    attr.set_fg(terminal::Color24bit(0x40, 0x50, 0x60));
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}\x40\x50\x60{}\x0f", fg24bit, bg8bit)) );
    CHECK( attr2.decode(enc) == 6 );
    CHECK( escape(attr2.encode()) == escape(enc) );

    attr.set_bg(terminal::Color24bit(0x70, 0x80, 0x90));
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}\x40\x50\x60{}\x70\x80\x90", fg24bit, bg24bit)) );

    attr.set_default_fg();
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}{}\x70\x80\x90", default_fg, bg24bit)) );

    attr.set_default_bg();
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}{}", default_fg, default_bg)) );

    attr.set_bold(true);
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}\x02{}{}", set_attrs, default_fg, default_bg)) );

    attr.set_italic(false);
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}\x02{}{}", set_attrs, default_fg, default_bg)) );
    
    attr.set_italic(true);
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{}\x03{}{}", set_attrs, default_fg, default_bg)) );
}


TEST_CASE( "Line::add_text", "[TextTerminal]" )
{
    TestRenderer r;
    terminal::Line line;
    terminal::Attributes bold, italic, attr;

    line.render(r);
    CHECK(r.output().empty());

    bold.set_bold(true);
    line.add_text(0, "bold", bold, /*insert=*/false);
    line.render(r);
    CHECK(r.output() == "[b]bold[r]");

    italic.set_italic(true);
    line.add_text(0, "italic", italic, /*insert=*/true);
    line.render(r);
    CHECK(r.output() == "[i]italic[b]bold[r]");

    line.add_text(2, "BOLD", bold, /*insert=*/false);
    line.render(r);
    CHECK(r.output() == "[i]it[b]BOLDbold[r]");

    line.add_text(20, "skipped after end", attr, /*insert=*/true);
    line.render(r);
    CHECK(r.output() == "[i]it[b]BOLDbold[r]          skipped after end");

    line.add_text(18, "#", attr, /*insert=*/false);
    line.render(r);
    CHECK(r.output() == "[i]it[b]BOLDbold[r]        # skipped after end");
}


TEST_CASE( "Line::erase_text", "[TextTerminal]" )
{
    TestRenderer r;
    terminal::Line line;
    terminal::Attributes bold, italic, attr;

    bold.set_bold(true);
    line.add_text(0, "verybold", bold, /*insert=*/false);
    line.render(r);
    CHECK(r.output() == "[b]verybold[r]");

    italic.set_italic(true);
    line.erase_text(3, 3, italic);
    line.render(r);
    CHECK(r.output() == "[b]ver[i]   [b]ld[r]");
}
