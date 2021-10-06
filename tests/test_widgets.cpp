// test_widgets.cpp created on 2018-08-06 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <catch2/catch.hpp>

#include <xci/widgets/TextTerminal.h>
#include <xci/core/string.h>
#include <xci/compat/macros.h>

#include <fmt/core.h>

using namespace xci::widgets;
using namespace xci::text;
using namespace xci::graphics;
using namespace xci::core;
using fmt::format;


class TestRenderer: public terminal::Renderer {
public:
    void set_font_style(FontStyle font_style) override {
        if (font_style == m_font_style)
            return;
        m_output.append([font_style](){
            switch (font_style) {
                case FontStyle::Regular:    return "[r]";
                case FontStyle::Italic:     return "[i]";
                case FontStyle::Bold:       return "[b]";
                case FontStyle::BoldItalic: return "[bi]";
            }
            UNREACHABLE;
        }());
        m_font_style = font_style;
    }
    void set_decoration(terminal::Decoration decoration) override {
        using terminal::Decoration;
        m_output.append([decoration](){
            switch (decoration) {
                case Decoration::None:          return "[ ]";
                case Decoration::Underlined:    return "[_]";
                case Decoration::Overlined:     return "[‾]";
                case Decoration::CrossedOut:    return "[-]";
            }
            UNREACHABLE;
        }());
    }
    void set_mode(terminal::Mode mode) override {
        using terminal::Mode;
        m_output.append([mode](){
            switch (mode) {
                case Mode::Normal:        return "[n]";
                case Mode::Bright:        return "[+]";
            }
            UNREACHABLE;
        }());
    }
    void set_default_fg_color() override {
        m_output.append(format("[fg:-]"));
    }
    void set_default_bg_color() override {
        m_output.append(format("[bg:-]"));
    }
    void set_fg_color(terminal::Color8bit fg) override {
        m_output.append(format("[fg:{:02x}]", fg));
    }
    void set_bg_color(terminal::Color8bit bg) override {
        m_output.append(format("[bg:{:02x}]", bg));
    }
    void set_fg_color(Color fg) override {
        m_output.append(format("[fg:{:02x}{:02x}{:02x}]", fg.r, fg.g, fg.b));
    }
    void set_bg_color(Color bg) override {
        m_output.append(format("[bg:{:02x}{:02x}{:02x}]", bg.r, bg.g, bg.b));
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
    using namespace xci::widgets::terminal::ctl;
    terminal::Attributes attr;
    terminal::Attributes attr2;
    std::string enc;

    CHECK( attr.encode().empty() );

    attr.set_fg(7);
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{:c}\x07", fg8bit)) );
    CHECK( attr2.decode(enc) == 2 );
    CHECK( escape(attr2.encode()) == escape(enc) );

    attr.set_bg(15);
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{:c}\x07{:c}\x0f", fg8bit, bg8bit)) );
    CHECK( attr2.decode(enc) == 4 );
    CHECK( escape(attr2.encode()) == escape(enc) );

    attr.set_fg(terminal::Color24bit(0x40, 0x50, 0x60));
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{:c}\x40\x50\x60{:c}\x0f", fg24bit, bg8bit)) );
    CHECK( attr2.decode(enc) == 6 );
    CHECK( escape(attr2.encode()) == escape(enc) );

    attr.set_bg(terminal::Color24bit(0x70, 0x80, 0x90));
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{:c}\x40\x50\x60{:c}\x70\x80\x90", fg24bit, bg24bit)) );

    attr.set_default_fg();
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{:c}{:c}\x70\x80\x90", default_fg, bg24bit)) );

    attr.set_default_bg();
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{:c}{:c}", default_fg, default_bg)) );

    attr.set_font_style(FontStyle::Italic);
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{:c}\x01{:c}{:c}", font_style, default_fg, default_bg)) );

    attr.set_font_style(FontStyle::Bold);
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{:c}\x02{:c}{:c}", font_style, default_fg, default_bg)) );

    attr.set_font_style(FontStyle::BoldItalic);
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{:c}\x03{:c}{:c}", font_style, default_fg, default_bg)) );

    attr.set_mode(terminal::Mode::Bright);
    enc = attr.encode();
    CHECK( escape(enc) == escape(format("{:c}\x03{:c}\x01{:c}{:c}", font_style, mode, default_fg, default_bg)) );
}


TEST_CASE( "Line::add_text", "[TextTerminal]" )
{
    TestRenderer r;
    terminal::Line line;
    terminal::Attributes bold, italic, attr;  // NOLINT

    line.render(r);
    CHECK(r.output().empty());

    bold.set_font_style(FontStyle::Bold);
    line.add_text(0, "bold", bold, false);
    line.render(r);
    CHECK(r.output() == "[b]bold[r]");

    italic.set_font_style(FontStyle::Italic);
    line.add_text(0, "italic", italic, true);
    line.render(r);
    CHECK(r.output() == "[i]italic[b]bold[r]");

    line.add_text(2, "BOLD", bold, false);
    line.render(r);
    CHECK(r.output() == "[i]it[b]BOLDbold[r]");

    line.add_text(20, "skipped after end", attr, true);
    line.render(r);
    CHECK(r.output() == "[i]it[b]BOLDbold[r]          skipped after end");

    line.add_text(18, "#", attr, false);
    line.render(r);
    CHECK(r.output() == "[i]it[b]BOLDbold[r]        # skipped after end");

    attr.set_fg(1);  // 8bit
    attr.set_bg(Color::Yellow());  // 24bit
    line.add_text(12, "@", attr, false);
    line.render(r);
    CHECK(r.output() == "[i]it[b]BOLDbold[r]  [fg:01][bg:ffff00]@[fg:-][bg:-]     # skipped after end");
}


TEST_CASE( "Line::erase_text", "[TextTerminal]" )
{
    TestRenderer r;
    terminal::Line line;
    terminal::Attributes bold, italic, attr;  // NOLINT

    bold.set_font_style(FontStyle::Bold);
    line.add_text(0, "verybold", bold, /*insert=*/false);
    line.render(r);
    CHECK(r.output() == "[b]verybold[r]");

    italic.set_font_style(FontStyle::Italic);
    line.erase_text(3, 3, italic);
    line.render(r);
    CHECK(r.output() == "[b]ver[i]   [b]ld[r]");
}
