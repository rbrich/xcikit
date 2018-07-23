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

namespace xci {
namespace graphics {


struct Color {
    constexpr Color() = default;
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
            : r(r), g(g), b(b), a(a) {}
    constexpr Color(int r, int g, int b, int a = 255)
            : r(uint8_t(r)), g(uint8_t(g)), b(uint8_t(b)), a(uint8_t(a)) {}
    constexpr Color(double r, double g, double b, double a = 1.0)
            : r(uint8_t(r*255)), g(uint8_t(g*255)), b(uint8_t(b*255)), a(uint8_t(a*255)) {}

    float red_f() const { return r / 255.f; }
    float green_f() const { return g / 255.f; }
    float blue_f() const { return b / 255.f; }
    float alpha_f() const { return a / 255.f; }

    // Predefined colors
    static constexpr Color Transparent() { return {0, 0, 0, 0}; }
    static constexpr Color Black() { return {0, 0, 0}; }
    static constexpr Color White() { return {255, 255, 255}; }
    static constexpr Color Red() { return {255, 0, 0}; }
    static constexpr Color Yellow() { return {255, 255, 0}; }

    uint8_t r = 0;    // red
    uint8_t g = 0;    // green
    uint8_t b = 0;    // blue
    uint8_t a = 255;  // alpha
};


}} // namespace xci::graphics

#endif // XCI_GRAPHICS_COLOR_H
