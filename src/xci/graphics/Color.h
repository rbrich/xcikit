// Color.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_COLOR_H
#define XCI_GRAPHICS_COLOR_H

namespace xci {
namespace graphics {

struct Color {
    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
            : r(r), g(g), b(b), a(a) {}

    // Predefined colors
    class White {};
    Color(White) : Color(255, 255, 255) {}

    uint8_t r;  // red
    uint8_t g;  // green
    uint8_t b;  // blue
    uint8_t a;  // alpha
};

}} // namespace xci::graphics

#endif // XCI_GRAPHICS_COLOR_H
