// Color.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_COLOR_H
#define XCI_GRAPHICS_COLOR_H

namespace xci {
namespace graphics {

struct Color {
    Color() = default;
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}

    // Predefined colors
    static inline Color White() { return Color(255, 255, 255); }

    uint8_t r = 0;    // red
    uint8_t g = 0;    // green
    uint8_t b = 0;    // blue
    uint8_t a = 255;  // alpha
};

}} // namespace xci::graphics

#endif // XCI_GRAPHICS_COLOR_H
