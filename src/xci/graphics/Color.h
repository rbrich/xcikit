// Color.h created on 2018-03-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_GRAPHICS_COLOR_H
#define XCI_GRAPHICS_COLOR_H

#include <cstdint>
#include <tuple>

namespace xci::graphics {


/// RGBA color in 4x 8bit integer format

struct Color {
    constexpr Color() = default;

    /// Integer RGB
    /// Each component value has to be in 0 .. 255 range.
    constexpr Color(uint8_t r, uint8_t g, uint8_t b) noexcept
        : r(r), g(g), b(b) {}
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept
        : r(r), g(g), b(b), a(a) {}
    constexpr Color(int r, int g, int b) noexcept
        : r(uint8_t(r)), g(uint8_t(g)), b(uint8_t(b)) {}
    constexpr Color(int r, int g, int b, int a) noexcept
        : r(uint8_t(r)), g(uint8_t(g)), b(uint8_t(b)), a(uint8_t(a)) {}

    /// Float RGB
    /// Each component value has to be in 0.0 .. 1.0 range.
    constexpr Color(float r, float g, float b) noexcept
            : r(uint8_t(r*255)), g(uint8_t(g*255)), b(uint8_t(b*255)) {}
    constexpr Color(float r, float g, float b, float a) noexcept
            : r(uint8_t(r*255)), g(uint8_t(g*255)), b(uint8_t(b*255)), a(uint8_t(a*255)) {}
    constexpr Color(double r, double g, double b) noexcept
            : r(uint8_t(r*255)), g(uint8_t(g*255)), b(uint8_t(b*255)) {}
    constexpr Color(double r, double g, double b, double a) noexcept
        : r(uint8_t(r*255)), g(uint8_t(g*255)), b(uint8_t(b*255)), a(uint8_t(a*255)) {}

    /// Predefined color palette (compatible with 256 color xterm)
    explicit Color(uint8_t palette_index) noexcept;

    /// Predefined named colors
    static constexpr Color Transparent() { return {0, 0, 0, 0}; }
    static constexpr Color Black() { return {0, 0, 0}; }
    static constexpr Color White() { return {255, 255, 255}; }
    static constexpr Color Red() { return {255, 0, 0}; }
    static constexpr Color Green() { return {0, 255, 0}; }
    static constexpr Color Blue() { return {0, 0, 255}; }
    static constexpr Color Yellow() { return {255, 255, 0}; }

    // Access components as float values (0.0 .. 1.0)
    // See FloatColor below for conversion of whole Color to float[4] format
    constexpr float red_f() const { return float(r) / 255.f; }
    constexpr float green_f() const { return float(g) / 255.f; }
    constexpr float blue_f() const { return float(b) / 255.f; }
    constexpr float alpha_f() const { return float(a) / 255.f; }

    // Comparison operators
    constexpr bool operator==(Color rhs) const
        { return std::tie(r, g, b, a) == std::tie(rhs.r, rhs.g, rhs.b, rhs.a); }
    constexpr bool operator!=(Color rhs) const { return !(rhs == *this); }

    // Direct access to components
    uint8_t r = 0;    // red
    uint8_t g = 0;    // green
    uint8_t b = 0;    // blue
    uint8_t a = 255;  // alpha
};


/// RGBA color in 4x 32bit float format
/// (this format is used in GLSL shaders as vec4)

struct FloatColor {
    constexpr FloatColor(Color color)  // NOLINT (implicit conversion)
            : r(color.red_f())
            , g(color.green_f())
            , b(color.blue_f())
            , a(color.alpha_f()) {}

    // Direct access to components
    float r, g, b, a;
};


} // namespace xci::graphics

#endif // XCI_GRAPHICS_COLOR_H
