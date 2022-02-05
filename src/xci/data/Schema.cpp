// Schema.cpp created on 2022-02-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Schema.h"

namespace xci::data {


void Schema::add_member(uint8_t key, const char* name, std::string&& type)
{
    const size_t idx = m_group_stack.back().buffer.struct_idx;
    auto& members = m_structs[idx].members;
    Member m{key, name, std::move(type)};
    auto it = std::find(members.begin(), members.end(), m);
    if (it == members.end()) {
        members.emplace_back(std::move(m));
    }
}


} // namespace xci::data
