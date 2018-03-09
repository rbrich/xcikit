// Color.h created on 2018-03-04, part of XCI toolkit

#ifndef XCI_GRAPHICS_COLOR_H
#define XCI_GRAPHICS_COLOR_H

namespace xci {
namespace graphics {

struct Color {
    Color() = default;
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
            : r(r), g(g), b(b), a(a) {}
    Color(int r, int g, int b, int a = 255)
            : r(uint8_t(r)), g(uint8_t(g)), b(uint8_t(b)), a(uint8_t(a)) {}
    Color(double r, double g, double b, double a = 1.0)
            : r(uint8_t(r*255)), g(uint8_t(g*255)), b(uint8_t(b*255)), a(uint8_t(a*255)) {}

    float red_f() const { return r / 255.f; }
    float green_f() const { return g / 255.f; }
    float blue_f() const { return b / 255.f; }
    float alpha_f() const { return a / 255.f; }

    // Predefined colors
    static inline Color White() { return {255, 255, 255}; }

    uint8_t r = 0;    // red
    uint8_t g = 0;    // green
    uint8_t b = 0;    // blue
    uint8_t a = 255;  // alpha
};

}} // namespace xci::graphics

#endif // XCI_GRAPHICS_COLOR_H
