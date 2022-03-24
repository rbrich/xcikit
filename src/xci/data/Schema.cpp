// Schema.cpp created on 2022-02-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Schema.h"

namespace xci::data {


void Schema::add_member(uint8_t key, const char* name, std::string&& type)
{
    if (m_structs.empty())
        m_structs.emplace_back(Struct{"struct Main"});
    const size_t idx = m_group_stack.back().buffer.struct_idx;
    auto& members = m_structs[idx].members;
    Member m{key, (name == nullptr ? "" : name), std::move(type)};
    auto it = std::find(members.begin(), members.end(), m);
    if (it == members.end()) {
        members.emplace_back(std::move(m));
    }
}


void Schema::_enter_group(const std::type_info& ti, const std::string& prefix,
                          uint8_t key, const char* name)
{
    auto [it, add] = m_type_to_struct_idx.try_emplace(std::type_index(ti), 0);
    if (add) {
        it->second = m_structs.size();

        auto type_name = name_of_type(ti, name);
        type_name.insert(0, prefix);

        // Make sure the type name is unique
        if (struct_by_name(type_name)) {
            type_name += '_';
            type_name += std::to_string(it->second);
        }

        m_structs.emplace_back(std::move(type_name));
    }

    add_member(key, name, std::string{m_structs[it->second].name});

    m_group_stack.emplace_back();
    m_group_stack.back().buffer.struct_idx = it->second;
}


} // namespace xci::data
