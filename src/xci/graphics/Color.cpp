// Color.cpp created on 2018-08-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Color.h"
#include <xci/core/log.h>
#include <xci/core/string.h>
#include <array>
#include <string_view>
#include <sstream>

namespace xci::graphics {

using namespace xci::core;


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


struct NamedColor{
    constexpr NamedColor(std::string_view name, Color color) : name(name), color(color) {}
    std::string_view name;
    Color color;
};


static constexpr std::array c_named_colors = {
    NamedColor("Black", Color::Black()),
    NamedColor("White", Color::White()),
    NamedColor("Red", Color::Red()),
    NamedColor("Green", Color::Green()),
    NamedColor("Blue", Color::Blue()),
    NamedColor("Cyan", Color::Cyan()),
    NamedColor("Magenta", Color::Magenta()),
    NamedColor("Yellow", Color::Yellow()),
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


Color::Color(std::string_view spec)
{
    if (spec.starts_with('#')) {
        // parse hex digits
        spec = spec.substr(1);
        const auto len = spec.size();
        uint32_t val;
        std::istringstream(std::string(spec)) >> std::hex >> val;
        if (len == 1 || len == 2) {
            *this = Color(uint8_t(val));
            return;
        }
        if (len == 3 || len == 4) {
            if (len == 3)
                val = (val << 4) | 0xF;
            r = (val & 0xF000) >> 12;
            g = (val & 0x0F00) >> 8;
            b = (val & 0x00F0) >> 4;
            a = (val & 0x000F);
            r = (r << 4) | r;
            g = (g << 4) | g;
            b = (b << 4) | b;
            a = (a << 4) | a;
            return;
        }
        if (len == 6 || len == 8) {
            if (len == 6)
                val = (val << 8) | 0xFF;
            r = (val & 0xFF000000) >> 24;
            g = (val & 0x00FF0000) >> 16;
            b = (val & 0x0000FF00) >> 8;
            a = (val & 0x000000FF);
            return;
        }
    } else {
        // named colors
        for (const auto& nc : c_named_colors) {
            if (ci_equal(nc.name, spec)) {
                *this = nc.color;
                return;
            }
        }
    }

    // not matched
    log::error("Color: could not interpret \"{}\"", spec);
    *this = Color::Red();
}


} // namespace xci::graphics
