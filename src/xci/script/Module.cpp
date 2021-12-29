// Module.cpp created on 2019-06-12 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Module.h"
#include "Function.h"
#include "Error.h"

#include <xci/data/BinaryWriter.h>
#include <xci/data/BinaryReader.h>
#include <xci/core/string.h>

#include <fstream>

namespace xci::script {

using std::move;


Module::~Module()
{
    #ifdef TRACE_REFCOUNT
    std::cout << "* in ~Module " << name() << std::endl;
    #endif
    for (const auto& val : m_values) {
        val.decref();
    }
}


SymbolPointer Module::add_native_function(
        std::string&& name, std::vector<TypeInfo>&& params, TypeInfo&& retval,
        NativeDelegate native)
{
    Function fn {*this, symtab().add_child(name)};
    fn.signature().params = move(params);
    fn.signature().return_type = move(retval);
    fn.set_native(native);
    FunctionId fn_id = add_function(move(fn));
    return symtab().add({move(name), Symbol::Function, fn_id.index});
}


Index Module::get_imported_module_index(Module* module) const
{
    auto it = find(m_modules.begin(), m_modules.end(), module);
    if (it == m_modules.end())
        return no_index;
    return it - m_modules.begin();
}


auto Module::add_function(Function&& fn) -> FunctionId
{
    return m_functions.add(move(fn));
}


Index Module::add_value(TypedValue&& value)
{
    auto idx = find_value(value);
    if (idx != no_index) {
        value.decref();  // we don't save the new value -> release it
        return idx;
    }

    m_values.add(move(value));
    return m_values.size() - 1;
}


Index Module::find_value(const TypedValue& value) const
{
    auto it = std::find(m_values.begin(), m_values.end(), value);
    if (it == m_values.end())
        return no_index;
    return it - m_values.begin();
}


Index Module::add_type(TypeInfo type_info)
{
    assert(!type_info.is_generic());

    auto idx = find_type(type_info);
    if (idx != no_index)
        return idx;

    m_types.push_back(move(type_info));
    return m_types.size() - 1;
}


Index Module::find_type(const TypeInfo& type_info) const
{
    assert(!type_info.is_generic());
    auto it = std::find(m_types.begin(), m_types.end(), type_info);
    if (it == m_types.end())
        return no_index;
    return it - m_types.begin();
}


auto Module::add_class(Class&& cls) -> ClassId
{
    return m_classes.add(move(cls));
}


auto Module::add_instance(Instance&& inst) -> InstanceId
{
    return m_instances.add(move(inst));
}


SymbolTable& Module::symtab_by_qualified_name(std::string_view name)
{
    auto parts = core::split(name, "::");
    auto part_it = parts.begin();

    SymbolTable* symtab = nullptr;
    if (*part_it == m_symtab.name()) {
        // a symbol from this module
        symtab = &m_symtab;
    } else {
        // a symbol from an imported module
        for (Module* module : m_modules)
            if (module->name() == *part_it)
                symtab = &module->symtab();
        if (symtab == nullptr)
            throw UnresolvedSymbol(name);
    }

    while (++part_it != parts.end()) {
        symtab = symtab->find_child_by_name(*part_it);
        if (symtab == nullptr)
            throw UnresolvedSymbol(name);
    }
    return *symtab;
}


void Module::add_spec_function(SymbolPointer gen_fn, Index spec_fn_idx)
{
    m_spec_functions.emplace(gen_fn, spec_fn_idx);
}


std::vector<Index> Module::get_spec_functions(SymbolPointer gen_fn)
{
    auto [beg, end] = m_spec_functions.equal_range(gen_fn);
    std::vector<Index> res;
    res.reserve(std::distance(beg, end));
    std::transform(beg, end, std::back_inserter(res), [](auto item){
        return item.second;
    });
    return res;
}


bool Module::operator==(const Module& rhs) const
{
    return m_modules == rhs.m_modules &&
           m_functions == rhs.m_functions &&
           m_values == rhs.m_values;
}


bool Module::save_to_file(const std::string& filename)
{
    std::ofstream f(filename, std::ios::binary);
    xci::data::BinaryWriter writer(f, true);
    writer(m_modules, m_values, m_symtab, m_functions);
    return !f.fail();
}


bool Module::load_from_file(const std::string& filename)
{
    std::ifstream f(filename, std::ios::binary);
    xci::data::BinaryReader reader(f);
    reader(m_modules, m_values, m_symtab);
    reader.repeated(m_functions, *this);
    return !f.fail();
}


} // namespace xci::script
