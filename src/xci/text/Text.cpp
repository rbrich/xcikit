// Text.cpp created on 2018-03-02, part of XCI toolkit

#include "Text.h"

#include <xci/graphics/Sprite.h>
using namespace xci::graphics;

namespace xci {
namespace text {


void Text::draw(View& target, const Vec2f& pos) const
{
    m_font->set_size(m_size);
//    states.blendMode = sf::BlendAlpha;

    Vec2f pen = pos;
    for (auto code_point : m_string) {
        // handle new lines
        if (code_point == 10) {
            pen.x = pos.x;
            pen.y += m_font->scaled_line_height();
            continue;
        }

        auto glyph = m_font->get_glyph(code_point);
        if (glyph == nullptr)
            continue;

        Sprite sprite(m_font->get_texture(), glyph->tex_coords());
        //sprite.setColor(m_color);
        target.draw(sprite, {pen.x + glyph->scaled_base_x(),
                             pen.y - glyph->scaled_base_y()});

#if 0
        sf::RectangleShape bbox;
        sf::FloatRect m;
        m.left = ft_to_float(gm.horiBearingX) + pen;
        m.top = -ft_to_float(gm.horiBearingY);
        m.width = ft_to_float(gm.width);
        m.height = ft_to_float(gm.height);
        bbox.setPosition(m.left, m.top);
        bbox.setSize({m.width, m.height});
        bbox.setFillColor(sf::Color::Transparent);
        bbox.setOutlineColor(sf::Color::Blue);
        bbox.setOutlineThickness(1);
        target.draw(bbox, states);
#endif

        pen.x += glyph->scaled_advance();
    }
}

Text::Metrics Text::get_metrics() const
{
    Text::Metrics metrics;
    m_font->set_size(m_size);
    for (auto code_point : m_string) {
        auto glyph = m_font->get_glyph(code_point);

        // Expand text bounds by glyph bounds
        Rect_f m;
        m.x = metrics.advance.x + glyph->base_x();
        m.y = 0 - glyph->base_y();
        m.w = glyph->width();
        m.h = glyph->height();  // ft_to_float(gm.height)
        metrics.bounds = metrics.bounds.union_(m);

        metrics.advance.x += glyph->advance();
    }
    return metrics;
}


}} // namespace xci::text
