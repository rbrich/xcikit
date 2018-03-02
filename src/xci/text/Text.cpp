// Text.cpp created on 2018-03-02, part of XCI toolkit

#include "Text.h"

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace xci {
namespace text {


// Extend `rect` to contain `contained` rect.
template<typename T>
void extend_rect(sf::Rect<T>& rect, const sf::Rect<T>& contained) {
    auto rr = rect.left + rect.width;
    auto rb = rect.top + rect.height;
    auto cr = contained.left + contained.width;
    auto cb = contained.top + contained.height;
    rect.left = std::min(rect.left, contained.left);
    rect.top = std::min(rect.top, contained.top);
    rect.width = std::max(rr, cr) - rect.left;
    rect.height = std::max(rb, cb) - rect.top;
}


void Text::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    m_font->set_size(m_size);
    states.blendMode = sf::BlendAlpha;
    states.transform.scale(m_font->get_ratio(), m_font->get_ratio());

    sf::Vector2f pen;
    for (auto code_point : m_string) {
        // handle new lines
        if (code_point == 10) {
            pen.x = 0;
            pen.y += m_font->scaled_line_height();
            continue;
        }

        auto glyph = m_font->get_glyph(code_point);
        if (glyph == nullptr)
            continue;

        sf::Sprite sprite(m_font->get_texture(), glyph->tex_coords());
        sprite.setPosition(pen.x + glyph->scaled_base_x(),
                           pen.y - glyph->scaled_base_y());
        sprite.setColor(m_color);
        target.draw(sprite, states);

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
        sf::FloatRect m;
        m.left = metrics.advance.x + glyph->base_x();
        m.top = 0 - glyph->base_y();
        m.width = glyph->width();
        m.height = glyph->height();  // ft_to_float(gm.height)
        extend_rect(metrics.bounds, m);

        metrics.advance.x += glyph->advance();
    }
    return metrics;
}


}} // namespace xci::text
