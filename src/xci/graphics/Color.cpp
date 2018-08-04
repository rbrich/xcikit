// Color.cpp created on 2018-08-04, part of XCI toolkit
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

    // matrix of 216 colors (6*6*6)
    if (index < 232) {
        index -= 16;
        r = color_scale_6to256(index / 36);
        g = color_scale_6to256((index % 36) / 6);
        b = color_scale_6to256(index % 6);
        return;
    }

    // grayscale
    index -= 232;
    r = g = b = uint8_t(8 + 10 * index);
}


} // namespace xci::graphics
