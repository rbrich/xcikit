// type_index.cpp created on 2023-05-06 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "type_index.h"
#include <xci/script/Module.h>
#include <cassert>

namespace xci::script {


Index make_type_index(Module& mod, const TypeInfo& type_info)
{
    const Module& builtin_module = mod.module_manager().builtin_module();
    // try builtin module
    Index type_idx = builtin_module.find_type(type_info);
    if (type_idx != no_index)
        return type_idx << 7;
    // add to requested module, or find existing
    auto mod_idx = mod.module_manager().get_module_index(mod);
    assert(mod_idx != no_index);
    assert(mod_idx < 128);
    // first add underlying types
    if (type_info.is_named())
        make_type_index(mod, type_info.underlying());
    if (type_info.is_list())
        make_type_index(mod, type_info.elem_type());
    if (type_info.is_tuple())
        for (const TypeInfo& ti : type_info.subtypes())
            make_type_index(mod, ti);
    if (type_info.is_struct())
        for (const auto& item : type_info.struct_items())
            make_type_index(mod, item.second);
    return (mod.add_type(type_info) << 7) + mod_idx;
}


Index get_type_index(const ModuleManager& mm, const TypeInfo& type_info)
{
    for (Index mod_idx = 0; mod_idx != mm.num_modules(); ++mod_idx) {
        assert(mod_idx < 128);
        const Module& mod = mm.get_module(mod_idx);
        const Index type_idx = mod.find_type(type_info);
        if (type_idx != no_index)
            return (type_idx << 7) + mod_idx;
    }
    return no_index;
}


const TypeInfo& get_type_info(const ModuleManager& mm, int32_t type_idx)
{
    static const TypeInfo unknown = ti_unknown();
    if (type_idx < 0)
        return unknown;

    auto module_index = size_t(type_idx % 128);
    if (module_index >= mm.num_modules())
        return unknown;
    const Module& mod = mm.get_module(module_index);

    auto type_index = size_t(type_idx / 128);
    if (type_index >= mod.num_types())
        return unknown;
    return mod.get_type(type_index);
}


const TypeInfo& get_type_info_unchecked(const ModuleManager& mm, int32_t type_idx)
{
    assert(type_idx >= 0);
    auto module_index = size_t(type_idx % 128);
    auto type_index = size_t(type_idx / 128);

    assert(module_index < mm.num_modules());
    const Module& mod = mm.get_module(module_index);
    assert(type_index < mod.num_types());

    return mod.get_type(type_index);
}


}  // namespace xci::script
