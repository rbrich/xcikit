// Color.h created on 2018-03-04, part of XCI toolkit
// Copyright 2018 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef XCI_GRAPHICS_COLOR_H
#define XCI_GRAPHICS_COLOR_H

#include <cstdint>
#include <tuple>

namespace xci::graphics {


/// RGBA color

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
    float red_f() const { return r / 255.f; }
    float green_f() const { return g / 255.f; }
    float blue_f() const { return b / 255.f; }
    float alpha_f() const { return a / 255.f; }

    // Comparison operators
    bool operator==(const Color& rhs) const
        { return std::tie(r, g, b, a) == std::tie(rhs.r, rhs.g, rhs.b, rhs.a); }
    bool operator!=(const Color& rhs) const { return !(rhs == *this); }

    // Direct access to components
    uint8_t r = 0;    // red
    uint8_t g = 0;    // green
    uint8_t b = 0;    // blue
    uint8_t a = 255;  // alpha
};


} // namespace xci::graphics

#endif // XCI_GRAPHICS_COLOR_H
