// Text.h created on 2018-03-02, part of XCI toolkit

#ifndef XCI_TEXT_TEXT_H
#define XCI_TEXT_TEXT_H

#include <xci/text/Font.h>

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/String.hpp>

namespace xci {
namespace text {


// Text rendering
class Text: public sf::Drawable {
public:

    // Set string
    void set_string(const sf::String& string) { m_string = string; }

    // Font
    void set_font(Font* font) { m_font = font; }
    void set_font(Font& font) { m_font = &font; }

    // Size
    void set_size(unsigned size) { m_size = size; }
    unsigned size() const { return m_size; }

    // Color
    void set_color(const sf::Color &color) { m_color = color; }
    const sf::Color& color() const { return m_color; }

    // Measure text (metrics are affected by string, font, size)
    struct Metrics {
        sf::Vector2f advance;
        sf::FloatRect bounds;
    };
    Metrics get_metrics() const;

protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    sf::String m_string;
    Font* m_font;
    unsigned m_size = 12;
    sf::Color m_color = sf::Color::White;
};


}} // namespace xci::text

#endif // XCI_TEXT_TEXT_H
