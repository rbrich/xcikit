// Page.cpp created on 2018-03-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/text/layout/Page.h>
#include <xci/graphics/View.h>
#include <xci/core/string.h>
#include <xci/core/log.h>

#include <cassert>
#include <utility>

namespace xci::text { class Style; }

namespace xci::text::layout {

using namespace xci::core;
using namespace xci::graphics::unit_literals;
using xci::graphics::View;
using xci::graphics::Color;


Word::Word(Page& page, const std::string& utf8)
    : m_style(page.style())
{
    auto* font = m_style.font();
    if (!font) {
        assert(!"Font is not set!");
        return;
    }

    m_style.apply_view(page.target());
    auto scale = m_style.scale();

    m_baseline = page.target().size_to_viewport(FramebufferPixels{font->ascender() * scale});
    auto descender_vp = page.target().size_to_viewport(FramebufferPixels{font->descender() * scale});
    const auto font_height = m_baseline - descender_vp;

    // Measure word (metrics are affected by string, font, size)
    ViewportCoords pen;
    m_bbox = {0, ViewportUnits{0} - m_baseline, 0, font_height};

    m_shaped = font->shape_text(utf8);

    for (const auto& shaped_glyph : m_shaped) {
        auto* glyph = font->get_glyph(shaped_glyph.glyph_index);
        auto advance = page.target().size_to_viewport(FramebufferCoords{shaped_glyph.advance * scale});
        if (glyph != nullptr) {
            // Expand text bounds by glyph bounds
            ViewportRect rect{pen.x,
                              pen.y - m_baseline,
                              advance.x,
                              font_height};

            m_bbox.extend(rect);
        }

        pen += advance;
    }

    // Check line end
    if (page.width() > 0.0f && page.pen().x + pen.x > page.width()) {
        page.finish_line();
    }

    // Set position according to pen
    m_pos = page.pen();

    page.advance_pen(pen);
}


void Word::move_x(ViewportUnits offset)
{
    m_pos.x += offset;
}


void Word::update(const graphics::View& target)
{
    auto* font = m_style.font();
    if (!font) {
        assert(!"Font is not set!");
        return;
    }

    m_style.apply_view(target);
    auto scale = m_style.scale();

    auto& renderer = target.window()->renderer();

    const auto fb_1px = target.size_to_viewport(1_fb);
    m_debug_shapes.clear();
    m_sprites.reset();
    m_outline_sprites.reset();

    if (target.has_debug_flag(View::Debug::WordBBox)) {
        m_debug_shapes.emplace_back(renderer,
                Color(0, 150, 0),
                Color(50, 250, 50));
        m_debug_shapes.back().add_rectangle(m_bbox, fb_1px);
        m_debug_shapes.back().update();
    }

    bool show_bboxes = target.has_debug_flag(View::Debug::GlyphBBox);
    if (show_bboxes) {
        m_debug_shapes.emplace_back(renderer,
                Color(150, 0, 0),
                Color(250, 50, 50));
    }

    auto render_sprites = [&](std::optional<graphics::Sprites>& sprites, graphics::Color color) {
        sprites.emplace(renderer, font->texture(), color);

        ViewportCoords pen;
        for (const auto& shaped_glyph : m_shaped) {
            auto* glyph = font->get_glyph(shaped_glyph.glyph_index);
            auto advance = target.size_to_viewport(FramebufferCoords{shaped_glyph.advance * scale});
            auto offset = target.size_to_viewport(FramebufferSize{shaped_glyph.offset});
            if (glyph != nullptr) {
                auto bearing = target.size_to_viewport(FramebufferSize{glyph->bearing()});
                auto glyph_size = target.size_to_viewport(FramebufferSize{glyph->size()});
                ViewportRect rect{pen.x + (offset.x + bearing.x) * scale,
                                  pen.y + (offset.y - bearing.y) * scale,
                                  glyph_size.x * scale,
                                  glyph_size.y * scale};
                sprites->add_sprite(rect, glyph->tex_coords());
                if (show_bboxes)
                    m_debug_shapes.back().add_rectangle(rect, fb_1px);
            }
            pen += advance;
        }

        sprites->update();
    };

    if (!m_style.color().is_transparent()) {
        render_sprites(m_sprites, m_style.color());
    }

    if (!m_style.outline_color().is_transparent()) {
        m_style.apply_outline(target);
        render_sprites(m_outline_sprites, m_style.outline_color());
    }

    if (show_bboxes)
        m_debug_shapes.back().update();

    if (target.has_debug_flag(View::Debug::WordBasePoint)) {
        const auto sc_1px = target.size_to_viewport(1_sc);
        m_debug_shapes.emplace_back(renderer, Color(150, 0, 255));
        m_debug_shapes.back().add_rectangle({- sc_1px, - sc_1px, 2 * sc_1px, 2 * sc_1px});
        m_debug_shapes.back().update();
    }
}


void Word::draw(graphics::View& target, const ViewportCoords& pos) const
{
    for (auto& shape : m_debug_shapes) {
        shape.draw(target, m_pos + pos);
    }

    if (m_outline_sprites)
        m_outline_sprites->draw(target, m_pos + pos);

    if (m_sprites)
        m_sprites->draw(target, m_pos + pos);

    if (target.has_debug_flag(View::Debug::WordBasePoint)
    && !m_debug_shapes.empty()) {
        // basepoint needs to be drawn on-top (it's the last debug shape)
        m_debug_shapes.back().draw(target, m_pos + pos);
    }
}


// -----------------------------------------------------------------------------


const ViewportRect& Line::bbox() const
{
    if (m_bbox_valid)
        return m_bbox;
    // Refresh
    bool first = true;
    for (const auto* word : m_words) {
        if (first) {
            m_bbox = word->bbox();
            first = false;
        } else {
            m_bbox.extend(word->bbox());
        }
    }
    // Add padding
    if (m_padding != 0_vp) {
        m_bbox.x -= m_padding;
        m_bbox.y -= m_padding;
        m_bbox.w += 2 * m_padding;
        m_bbox.h += 2 * m_padding;
    }
    m_bbox_valid = true;
    return m_bbox;
}


ViewportUnits Line::baseline() const
{
    if (m_words.empty())
        return 0_vp;
    return m_words[0]->baseline();
}


void Line::align(Alignment alignment, ViewportUnits width)
{
    // This also computes bbox if not valid
    auto line_width = bbox().w - 2 * m_padding;
    if (line_width >= width)
        return;  // Not enough space for aligning
    auto current_x = m_bbox.x + m_padding;
    ViewportUnits target_x;
    switch (alignment) {
        case Alignment::Justify:  // not implemented, fallback to Left
        case Alignment::Left:
            target_x = 0.f;
            break;
        case Alignment::Right:
            target_x = width - line_width;
            break;
        case Alignment::Center:
            target_x = (width - line_width) / 2.f;
            break;
    }
    // Realign all Words
    auto offset = target_x - current_x;
    for (auto* word : m_words) {
        word->move_x(offset);
    }
    m_bbox.x += offset;
}


// -----------------------------------------------------------------------------


void Span::add_word(Word& word)
{
    // Add word to current line
    auto& line = m_parts.back();
    line.add_word(word);
}


void Span::adjust_style(const std::function<void(Style& word_style)>& fn_adjust)
{
    for (Line& part : m_parts) {
        for (Word* word : part.words()) {
            fn_adjust(word->style());
        }
    }
}


// -----------------------------------------------------------------------------


Page::Page()
{
    m_lines.emplace_back();
}


const graphics::View& Page::target() const
{
    if (!m_target) {
        assert(!"Target not set!");
        static graphics::View default_view;
        return default_view;
    }

    return *m_target;
}


void Page::clear()
{
    m_pen = {0.f, 0.f};
    m_pen_offset = {0.f, 0.f};
    m_style.clear();
    m_width = 0.f;
    m_alignment = Alignment::Left;
    m_tab_stops.clear();
    m_lines.clear();
    m_lines.emplace_back();
    m_spans.clear();
    m_words.clear();
}


void Page::add_tab_stop(ViewportUnits x)
{
    m_tab_stops.push_back(x);
    std::sort(m_tab_stops.begin(), m_tab_stops.end());
}


void Page::finish_line()
{
    // If the current line has any content...
    if (!m_lines.back().is_empty()) {
        // Apply alignment
        m_lines.back().align(m_alignment, m_width);
        // Add new line
        m_lines.emplace_back();
    }
    // Add new part to open spans
    for (auto& span_pair : m_spans) {
        auto& span = span_pair.second;
        if (span.is_open()) {
            span.add_part();
        }
    }
    // Move pen
    m_pen.x = 0;
    advance_line();
}


void Page::advance_line(float lines)
{
    m_style.apply_view(target());
    auto height = target().size_to_viewport(FramebufferPixels{m_style.font()->height() * m_style.scale()});
    m_pen.y += lines * height;
}


void Page::add_space(float spaces)
{
    m_pen.x += space_width() * spaces;
}


void Page::add_tab()
{
    // apply tab stops
    auto tab_stop = m_tab_stops.begin();
    ViewportUnits x = 0_vp;
    while (x <= m_pen.x && tab_stop != m_tab_stops.end()) {
        x = *tab_stop++;
    }
    // apply generic tabs
    if (x <= m_pen.x) {
        ViewportUnits tab_size = 8 * space_width();
        while (x <= m_pen.x)
            x += tab_size;
    }
    // move to new position
    m_pen.x = x;
}


void Page::add_word(const std::string& string)
{
    m_words.emplace_back(*this, string);

    // Add word to current line
    auto& line = m_lines.back();
    line.add_word(m_words.back());

    // Add word to open spans
    for (auto& span_pair : m_spans) {
        auto& span = span_pair.second;
        if (span.is_open()) {
            span.add_word(m_words.back());
        }
    }
}


ViewportUnits Page::space_width()
{
    m_style.apply_view(target());
    auto* glyph = m_style.font()->get_glyph_for_char(' ');
    return target().size_to_viewport(FramebufferPixels{glyph->advance() * m_style.scale()});
}


bool Page::begin_span(const std::string& name)
{
    auto result = m_spans.emplace(name, Span());
    if (!result.second) {
        log::error("Page: Span '{}' already exists!", name);
        return false;
    }
    return true;
}


bool Page::end_span(const std::string& name)
{
    auto iter = m_spans.find(name);
    if (iter == m_spans.end()) {
        log::error("Page: Span '{}' does not exists!", name);
        return false;
    }
    if (!iter->second.is_open()) {
        log::error("Page: Span '{}' is not open!", name);
        return false;
    }
    iter->second.close();
    return true;
}


Span* Page::get_span(const std::string& name)
{
    auto iter = m_spans.find(name);
    if (iter == m_spans.end())
        return nullptr;  // does not exist
    return &iter->second;
}


void Page::foreach_word(const std::function<void(Word& word)>& cb)
{
    if (!cb) return;
    for (auto& word : m_words) {
        cb(word);
    }
}


void Page::foreach_word(const std::function<void(const Word& word)>& cb) const
{
    if (!cb) return;
    for (const auto& word : m_words) {
        cb(word);
    }
}


void Page::foreach_line(const std::function<void(const Line& line)>& cb) const
{
    if (!cb) return;
    for (const auto& line : m_lines) {
        cb(line);
    }
}


void Page::foreach_span(const std::function<void(const Span& span)>& cb) const
{
    if (!cb) return;
    for (const auto& pair : m_spans) {
        cb(pair.second);
    }
}


} // namespace xci::text::layout
