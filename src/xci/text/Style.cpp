// Style.cpp created on 2018-03-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Style.h"

void xci::text::Style::clear()
{
    m_font = nullptr;
    m_size = 0.05f;
    m_color = graphics::Color::White();
}
