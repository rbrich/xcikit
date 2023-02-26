// Shape.cpp created on 2018-04-04 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Shape.h"

namespace xci::graphics {


void UniformColorShape::update(Color fill_color, Color outline_color,
                               float softness, float antialiasing)
{
    if (!m_primitives.empty()) {
        m_primitives.clear_uniforms();
        m_primitives.add_uniform(1, fill_color, outline_color);
        m_primitives.add_uniform(2, softness, antialiasing);
        m_primitives.set_shader(m_shader);
        m_primitives.set_blend(BlendFunc::AlphaBlend);
        m_primitives.update();
    }
}


void VaryingColorShape::update(float softness, float antialiasing)
{
    if (!m_primitives.empty()) {
        m_primitives.clear_uniforms();
        m_primitives.add_uniform(1, softness, antialiasing);
        m_primitives.set_shader(m_shader);
        m_primitives.set_blend(BlendFunc::AlphaBlend);
        m_primitives.update();
    }
}


} // namespace xci::graphics
