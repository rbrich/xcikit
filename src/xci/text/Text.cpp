// Text.cpp created on 2018-03-02, part of XCI toolkit

#include "Text.h"

#include <xci/graphics/Sprites.h>
using namespace xci::graphics;

namespace xci {
namespace text {


void Text::draw(View& target, const Vec2f& pos) const
{
    m_font->set_size(m_size);

    Sprites sprites(m_font->get_texture());

    Vec2f pen;
    for (CodePoint code_point : m_string) {
        // handle new lines
        if (code_point == 10) {
            pen.x = 0;
            pen.y += m_font->line_height();
            continue;
        }

        auto glyph = m_font->get_glyph(code_point);
        if (glyph == nullptr)
            continue;

        sprites.add_sprite({pen.x + glyph->base_x(),
                            pen.y - glyph->base_y()},
                           glyph->tex_coords(),
                           m_color);

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

        pen.x += glyph->advance();
    }

    sprites.draw(target, pos);
}

Text::Metrics Text::get_metrics() const
{
    Text::Metrics metrics;
    m_font->set_size(m_size);
    for (CodePoint code_point : m_string) {
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
