// Module.cpp created on 2019-06-12 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Module.h"
#include "Function.h"
#include "Error.h"

namespace xci::script {

using std::move;


Module::~Module()
{
    for (const auto& val : m_values) {
        val.decref();
    }
}


SymbolPointer Module::add_native_function(
        std::string&& name, std::vector<TypeInfo>&& params, TypeInfo&& retval,
        NativeDelegate native)
{
    auto fn = std::make_unique<Function>(*this, symtab().add_child(name));
    fn->signature().params = move(params);
    fn->signature().return_type = move(retval);
    fn->set_native(native);
    Index index = add_function(move(fn));
    return symtab().add({move(name), Symbol::Function, index});
}


Index Module::get_imported_module_index(Module* module) const
{
    auto it = find(m_modules.begin(), m_modules.end(), module);
    if (it == m_modules.end())
        return no_index;
    return it - m_modules.begin();
}


Index Module::add_function(std::unique_ptr<Function>&& fn)
{
    m_functions.push_back(move(fn));
    return m_functions.size() - 1;
}


Index Module::add_value(TypedValue&& value)
{
    m_values.add(move(value));
    return m_values.size() - 1;
}


Index Module::add_type(TypeInfo type_info)
{
    if (!type_info.is_unknown()) {
        auto idx = find_type(type_info);
        if (idx != no_index)
            return idx;
    }
    m_types.push_back(move(type_info));
    return m_types.size() - 1;
}


Index Module::find_type(const TypeInfo& type_info) const
{
    assert(!type_info.is_unknown());
    auto it = std::find(m_types.begin(), m_types.end(), type_info);
    if (it == m_types.end())
        return no_index;
    return it - m_types.begin();
}


Index Module::add_class(std::unique_ptr<Class>&& cls)
{
    m_classes.push_back(move(cls));
    return m_classes.size() - 1;
}


Index Module::add_instance(std::unique_ptr<Instance>&& inst)
{
    m_instances.push_back(move(inst));
    return m_instances.size() - 1;
}


bool Module::operator==(const Module& rhs) const
{
    return m_modules == rhs.m_modules &&
           m_functions == rhs.m_functions &&
           m_values == rhs.m_values;
}


} // namespace xci::script
