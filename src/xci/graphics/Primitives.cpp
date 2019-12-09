// Primitives.cpp created on 2018-08-03, part of XCI toolkit
// Copyright 2018, 2019 Radek Brich
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

#include "Primitives.h"
#include <xci/graphics/View.h>


namespace xci::graphics {


namespace {
    struct FloatColor {
        float r, g, b, a;
        FloatColor(const Color& color)  // NOLINT
                : r(color.red_f())
                , g(color.green_f())
                , b(color.blue_f())
                , a(color.alpha_f()) {}
    };
}


void Primitives::draw(View& view, const ViewportCoords& pos)
{
    view.push_offset(pos);
    draw(view);
    view.pop_offset();
}


void Primitives::set_uniform(uint32_t binding, float f1, float f2)
{
    struct { float f1, f2; } buf { f1, f2 };
    set_uniform_data(binding, &buf, sizeof(buf));
}


void Primitives::set_uniform(uint32_t binding, const Color& color)
{
    FloatColor buf {color};
    set_uniform_data(binding, &buf, sizeof(buf));
}


void Primitives::set_uniform(uint32_t binding, const Color& color1, const Color& color2)
{
    struct { FloatColor c1, c2; } buf { color1, color2 };
    set_uniform_data(binding, &buf, sizeof(buf));
}


} // namespace xci::graphics
