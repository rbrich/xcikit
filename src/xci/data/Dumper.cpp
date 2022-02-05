// Dumper.cpp created on 2019-03-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Dumper.h"

namespace xci::data {


void Dumper::write_key_name(uint8_t key, const char* name, char sep)
{
    m_stream << indent() << '(' << int(key) << ")";
    if (name != nullptr)
        m_stream << ' ' << name;
    m_stream << ':' << sep;
}


} // namespace xci::data
