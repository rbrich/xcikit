// Color.cpp created on 2018-08-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Color.h"

namespace xci::graphics {


// Basic palette of 4-bit colors
static constexpr Color c_colors_4bit[16] = {
        {0,0,0},        // 0 - black
        {178,23,23},    // 1 - red
        {23,178,23},    // 2 - green
        {178,103,23},   // 3 - yellow
        {23,23,178},    // 4 - blue
        {178,23,178},   // 5 - magenta
        {23,178,178},   // 6 - cyan
        {178,178,178},  // 7 - white
        {104,104,104},  // 8 - bright black
        {255,84,84},    // 9 - bright red
        {84,255,84},    // 10 - bright green
        {255,255,84},   // 11 - bright yellow
        {84,84,255},    // 12 - bright blue
        {255,84,255},   // 13 - bright magenta
        {84,255,255},   // 14 - bright cyan
        {255,255,255},  // 15 - bright white
};


static inline uint8_t color_scale_6to256(int value)
{
    return uint8_t(value == 0 ? 0 : 55 + value * 40);
}


Color::Color(uint8_t index) noexcept
{
    // basic 4-bit colors - lookup table
    if (index < 16) {
        *this = c_colors_4bit[index];
        return;
    }

    // 216 colors (6*6*6 cube)
    if (index < 232) {
        index -= 16;
        r = color_scale_6to256(index / 36);
        g = color_scale_6to256((index % 36) / 6);
        b = color_scale_6to256(index % 6);
        return;
    }

    // 24 step grayscale
    index -= 232;
    r = g = b = uint8_t(8 + 10 * index);
}


} // namespace xci::graphics
