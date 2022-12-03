// Element.h created on 2018-03-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_LAYOUT_PAGE_ELEMENT_H
#define XCI_TEXT_LAYOUT_PAGE_ELEMENT_H

#include "Page.h"

#include <string>

namespace xci::text::layout {

using graphics::VariUnits;
using graphics::VariSize;


class Element {
public:
    virtual ~Element() = default;
    virtual void apply(Page& page) = 0;
};


// ------------------------------------------------------------------------
// Control elements - change page attributes


class SetPageWidth: public Element {
public:
    explicit SetPageWidth(VariUnits width) : m_width(width) {}
    void apply(Page& page) override {
        page.set_width(page.target().to_fb(m_width));
    }

private:
    VariUnits m_width;
};


class SetAlignment: public Element {
public:
    explicit SetAlignment(Alignment alignment) : m_alignment(alignment) {}
    void apply(Page& page) override {
        page.set_alignment(m_alignment);
    }

private:
    Alignment m_alignment;
};


class AddTabStop: public Element {
public:
    explicit AddTabStop(VariUnits tab_stop) : m_tab_stop(tab_stop) {}
    void apply(Page& page) override {
        page.add_tab_stop(page.target().to_fb(m_tab_stop));
    }

private:
    VariUnits m_tab_stop;
};


class ResetTabStops: public Element {
public:
    void apply(Page& page) override {
        page.reset_tab_stops();
    }
};


class SetOffset: public Element {
public:
    explicit SetOffset(VariSize offset) : m_offset(offset) {}
    void apply(Page& page) override {
        page.set_pen_offset(page.target().to_fb(m_offset));
    }

private:
    VariSize m_offset;
};


class SetFont: public Element {
public:
    explicit SetFont(Font* font) : m_font(font) {}
    void apply(Page& page) override {
        page.set_font(m_font);
    }

private:
    Font* m_font;
};


class SetFontSize: public Element {
public:
    explicit SetFontSize(VariUnits size) : m_size(size) {}
    void apply(Page& page) override {
        page.set_font_size(m_size);
    }

private:
    VariUnits m_size;
};


class SetFontStyle: public Element {
public:
    explicit SetFontStyle(FontStyle font_style) : m_font_style(font_style) {}
    void apply(Page& page) override {
        page.set_font_style(m_font_style);
    }

private:
    FontStyle m_font_style;
};


class SetBold: public Element {
public:
    explicit SetBold(bool bold) : m_bold(bold) {}
    void apply(Page& page) override {
        auto style = static_cast<unsigned>(page.style().font_style());
        auto bold = static_cast<unsigned>(FontStyle::Bold);
        if (m_bold)
            style |= bold;
        else
            style &= ~bold;
        page.set_font_style(static_cast<FontStyle>(style));
    }

private:
    bool m_bold;
};


class SetItalic: public Element {
public:
    explicit SetItalic(bool italic) : m_italic(italic) {}
    void apply(Page& page) override {
        auto style = static_cast<unsigned>(page.style().font_style());
        auto italic = static_cast<unsigned>(FontStyle::Italic);
        if (m_italic)
            style |= italic;
        else
            style &= ~italic;
        page.set_font_style(static_cast<FontStyle>(style));
    }

private:
    bool m_italic;
};


class SetColor: public Element {
public:
    explicit SetColor(graphics::Color color) : m_color(color) {}
    void apply(Page& page) override {
        page.set_color(m_color);
    }

private:
    graphics::Color m_color;
};


// ------------------------------------------------------------------------
// Text elements


// Single word, consisting of letters (glyphs), font and style.
class AddWord: public Element {
public:
    explicit AddWord(std::string string) : m_string(std::move(string)) {}
    void apply(Page& page) override {
        page.add_word(m_string);
    }

private:
    std::string m_string;
};


class AddSpace: public Element {
public:
    void apply(Page& page) override {
        page.add_space();
    }
};


class AddTab: public Element {
public:
    void apply(Page& page) override {
        page.add_tab();
    }
};


class FinishLine: public Element {
public:
    void apply(Page& page) override {
        page.finish_line();
    }
};


class AdvanceLine: public Element {
public:
    explicit AdvanceLine(float lines) : m_lines(lines) {}
    void apply(Page& page) override {
        page.advance_line(m_lines);
    }

private:
    float m_lines;
};


class BeginSpan: public Element {
public:
    explicit BeginSpan(std::string name) : m_name(std::move(name)) {}
    void apply(Page& page) override {
        page.begin_span(m_name);
    }

private:
    std::string m_name;
};


class EndSpan: public Element {
public:
    explicit EndSpan(std::string name) : m_name(std::move(name)) {}
    void apply(Page& page) override {
        page.end_span(m_name);
    }

private:
    std::string m_name;
};


} // namespace xci::text::layout

#endif // XCI_TEXT_LAYOUT_PAGE_ELEMENT_H
