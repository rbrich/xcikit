// Element.h created on 2018-03-18, part of XCI toolkit
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

#ifndef XCI_TEXT_LAYOUT_PAGE_ELEMENT_H
#define XCI_TEXT_LAYOUT_PAGE_ELEMENT_H

#include "Page.h"

#include <string>

namespace xci {
namespace text {
namespace layout {


class Element {
public:
    virtual void apply(Page& page) = 0;
    virtual void draw(graphics::View& target, const util::Vec2f& pos) const {}
};


// ------------------------------------------------------------------------
// Control elements - change page attributes


class SetPageWidth: public Element {
public:
    explicit SetPageWidth(float width) : m_width(width) {}
    void apply(Page& page) override {
        page.set_width(m_width);
    }

private:
    float m_width;
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
    explicit AddTabStop(float tab_stop) : m_tab_stop(tab_stop) {}
    void apply(Page& page) override {
        page.add_tab_stop(m_tab_stop);
    }

private:
    float m_tab_stop;
};


class ResetTabStops: public Element {
public:
    void apply(Page& page) override {
        page.reset_tab_stops();
    }
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
    explicit SetFontSize(float size) : m_size(size) {}
    void apply(Page& page) override {
        page.set_font_size(m_size);
    }

private:
    float m_size;
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
class Word: public Element {
public:
    explicit Word(std::string string) : m_string(std::move(string)) {}

    void apply(Page& page) override;
    void draw(graphics::View& target, const util::Vec2f& pos) const override;

private:
    std::string m_string;
    Style m_style;
    util::Vec2f m_pos;  // relative to page origin (top-left corner)
};


class Space: public Element {
public:
    void apply(Page& page) override {
        page.add_space();
    }
};


class Tab: public Element {
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


}}} // namespace xci::text::layout

#endif // XCI_TEXT_LAYOUT_PAGE_ELEMENT_H
