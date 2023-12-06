// Page.cpp created on 2018-03-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/text/layout/Page.h>
#include <xci/graphics/View.h>
#include <xci/core/string.h>
#include <xci/core/log.h>

#include <cassert>
#include <utility>
#include <algorithm>

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

    m_baseline = FramebufferPixels{font->ascender() * scale};
    auto descender_vp = FramebufferPixels{font->descender() * scale};
    const auto font_height = m_baseline - descender_vp;

    // Measure word (metrics are affected by string, font, size)
    FramebufferCoords pen;
    m_bbox = {0, 0 - m_baseline, 0, font_height};

    m_shaped = font->shape_text(utf8);

    for (const auto& shaped_glyph : m_shaped) {
        auto* glyph = font->get_glyph(shaped_glyph.glyph_index);
        auto advance = FramebufferCoords{shaped_glyph.advance * scale};
        if (glyph != nullptr) {
            // Expand text bounds by glyph bounds
            const FramebufferRect rect{
                    pen.x,
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
        page.advance_line();
    }

    // Set position according to pen
    m_pos = page.pen();

    page.advance_pen(pen);
}


void Word::move_x(FramebufferPixels offset)
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

    m_debug_rects.clear();
    m_sprites.reset();
    m_outline_sprites.reset();

    if (target.has_debug_flag(View::Debug::WordBBox)) {
        m_debug_rects.emplace_back(renderer);
        m_debug_rects.back().add_rectangle(m_bbox, 1_fb);
        m_debug_rects.back().update(Color(0, 150, 0),
                                    Color(50, 250, 50));
    }

    bool show_bboxes = target.has_debug_flag(View::Debug::GlyphBBox);
    if (show_bboxes) {
        m_debug_rects.emplace_back(renderer);
    }

    auto render_sprites = [&](std::optional<graphics::Sprites>& sprites, graphics::Color color) {
        sprites.emplace(renderer, font->texture(), color);

        FramebufferCoords pen;
        for (const auto& shaped_glyph : m_shaped) {
            auto* glyph = font->get_glyph(shaped_glyph.glyph_index);
            auto advance = FramebufferCoords{shaped_glyph.advance * scale};
            auto offset = FramebufferSize{shaped_glyph.offset};
            if (glyph != nullptr) {
                auto bearing = FramebufferSize{glyph->bearing()};
                auto glyph_size = FramebufferSize{glyph->size()};
                FramebufferRect rect{pen.x + (offset.x + bearing.x) * scale,
                                     pen.y + (offset.y - bearing.y) * scale,
                                     glyph_size.x * scale,
                                     glyph_size.y * scale};
                sprites->add_sprite(rect, glyph->tex_coords());
                if (show_bboxes)
                    m_debug_rects.back().add_rectangle(rect, 1_fb);
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

    if (show_bboxes) {
        m_debug_rects.back().update(Color(150, 0, 0),
                                    Color(250, 50, 50));
    }

    if (target.has_debug_flag(View::Debug::WordBasePoint)) {
        const auto fb_1px = target.px_to_fb(1_px);
        m_debug_rects.emplace_back(renderer);
        m_debug_rects.back().add_rectangle({- fb_1px, - fb_1px, 2 * fb_1px, 2 * fb_1px});
        m_debug_rects.back().update(Color(150, 0, 255));
    }
}


void Word::draw(graphics::View& target, FramebufferCoords pos) const
{
    for (auto& rects : m_debug_rects) {
        rects.draw(target, m_pos + pos);
    }

    if (m_outline_sprites)
        m_outline_sprites->draw(target, m_pos + pos);

    if (m_sprites)
        m_sprites->draw(target, m_pos + pos);

    if (target.has_debug_flag(View::Debug::WordBasePoint)
    && !m_debug_rects.empty()) {
        // basepoint needs to be drawn on-top (it's the last debug shape)
        m_debug_rects.back().draw(target, m_pos + pos);
    }
}


// -----------------------------------------------------------------------------


const FramebufferRect& Line::bbox() const
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
    if (m_padding != 0_fb) {
        m_bbox.x -= m_padding;
        m_bbox.y -= m_padding;
        m_bbox.w += 2 * m_padding;
        m_bbox.h += 2 * m_padding;
    }
    m_bbox_valid = true;
    return m_bbox;
}


FramebufferPixels Line::baseline() const
{
    if (m_words.empty())
        return 0_fb;
    return m_words[0]->baseline();
}


void Line::align(Alignment alignment, FramebufferPixels width)
{
    // This also computes bbox if not valid
    auto line_width = bbox().w - 2 * m_padding;
    if (line_width >= width)
        return;  // Not enough space for aligning
    auto current_x = m_bbox.x + m_padding;
    FramebufferPixels target_x;
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


bool Span::contains(FramebufferCoords point) const
{
    return std::any_of(m_parts.begin(), m_parts.end(),
               [&point](const Line& line) { return line.bbox().contains(point); });
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


void Page::add_tab_stop(FramebufferPixels x)
{
    m_tab_stops.push_back(x);
    std::sort(m_tab_stops.begin(), m_tab_stops.end());
}


void Page::finish_line()
{
    if (m_lines.back().is_empty())
        return;  // already at a new line

    // Apply alignment
    m_lines.back().align(m_alignment, m_width);
    // Add new line
    m_lines.emplace_back();
    // Add new part to open spans
    for (auto& span : m_spans) {
        if (span.is_open()) {
            span.add_part();
        }
    }
    // Move pen
    m_pen.x = m_origin.x;
}


void Page::advance_line(float lines)
{
    m_style.apply_view(target());
    auto height = FramebufferPixels{m_style.font()->height() * m_style.scale()};
    m_pen.y += lines * m_line_spacing * height;
}


void Page::add_space(float spaces)
{
    m_pen.x += space_width() * spaces;
}


void Page::add_tab()
{
    // apply tab stops
    auto tab_stop = m_tab_stops.begin();
    FramebufferPixels x = 0;
    while (x <= m_pen.x && tab_stop != m_tab_stops.end()) {
        x = m_origin.x + *tab_stop++;
    }
    // apply generic tabs
    if (x <= m_pen.x) {
        const FramebufferPixels tab_size = 8 * space_width();
        if (tab_size > 0.f)
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
    for (auto& span : m_spans) {
        if (span.is_open()) {
            span.add_word(m_words.back());
        }
    }
}


FramebufferPixels Page::space_width()
{
    m_style.apply_view(target());
    auto* glyph = m_style.font()->get_glyph_for_char(' ');
    return FramebufferPixels{glyph->advance().x * m_style.scale()};
}


auto Page::begin_span() -> SpanIndex
{
    m_spans.emplace_back();
    return m_spans.size() - 1;
}


bool Page::end_span(SpanIndex index)
{
    if (index >= m_spans.size()) {
        log::error("Page::end_span: Span {} does not exists!", index);
        return false;
    }
    auto& span = m_spans[index];
    if (!span.is_open()) {
        log::error("Page::end_span: Span {} is not open!", index);
        return false;
    }
    span.close();
    return true;
}


Span* Page::get_span(SpanIndex index)
{
    if (index >= m_spans.size()) {
        log::error("Page::get_span: Span {} does not exists!", index);
        return nullptr;
    }
    return &m_spans[index];
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
    for (const auto& span : m_spans) {
        cb(span);
    }
}


} // namespace xci::text::layout
