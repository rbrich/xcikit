// Text.h created on 2018-03-02, part of XCI toolkit

#ifndef XCI_TEXT_TEXT_H
#define XCI_TEXT_TEXT_H

#include <xci/text/Font.h>

#include <xci/graphics/Color.h>
#include <xci/graphics/View.h>
using xci::graphics::Color;
using xci::graphics::View;

#include <xci/util/geometry.h>
using xci::util::Vec2f;
using xci::util::Rect_f;

#include <string>

namespace xci {
namespace text {


// Text rendering
class Text {
public:

    // Set string
    void set_string(const std::string& string) { m_string = string; }

    // Font
    void set_font(Font* font) { m_font = font; }
    void set_font(Font& font) { m_font = &font; }

    // Size
    void set_size(unsigned size) { m_size = size; }
    unsigned size() const { return m_size; }

    // Color
    void set_color(const Color& color) { m_color = color; }
    const Color& color() const { return m_color; }

    // Measure text (metrics are affected by string, font, size)
    struct Metrics {
        Vec2f advance;
        Rect_f bounds;
    };
    Metrics get_metrics() const;

    void draw(View& target, const Vec2f& pos) const;

private:
    std::string m_string;
    Font* m_font;
    unsigned m_size = 12;
    Color m_color = Color::White();
};


}} // namespace xci::text

#endif // XCI_TEXT_TEXT_H
