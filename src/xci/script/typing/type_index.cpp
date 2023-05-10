// type_index.cpp created on 2023-05-06 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "type_index.h"
#include <xci/script/Module.h>


namespace xci::script {


Index make_type_index(Module& mod, const TypeInfo& type_info)
{
    const Module& builtin_module = mod.module_manager().builtin_module();
    Index type_idx = builtin_module.find_type(type_info);
    if (type_idx >= 32) {  // includes no_index
        type_idx = 32 + mod.add_type(type_info);
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
    }
    return type_idx;
}


Index get_type_index(const Module& mod, const TypeInfo& type_info)
{
    // is the type builtin?
    const Module& builtin_module = mod.module_manager().builtin_module();
    Index type_idx = builtin_module.find_type(type_info);
    if (type_idx >= 32) {
        type_idx = mod.find_type(type_info);
        if (type_idx != no_index)
            type_idx += 32;
    }
    return type_idx;
}


const TypeInfo& get_type_info(const Module& mod, int32_t type_idx)
{
    static const TypeInfo unknown = ti_unknown();
    if (type_idx < 0)
        return unknown;
    if (type_idx < 32) {
        // builtin module
        const Module& builtin = mod.get_imported_module(0);
        if ((size_t)type_idx >= builtin.num_types())
            return unknown;
        return builtin.get_type(type_idx);
    }
    type_idx -= 32;
    if ((size_t)type_idx >= mod.num_types())
        return unknown;
    return mod.get_type(type_idx);
}


}  // namespace xci::script
