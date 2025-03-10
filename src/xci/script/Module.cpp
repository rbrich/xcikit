// Module.cpp created on 2019-06-12 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Module.h"
#include "Function.h"
#include "Error.h"
#include "ast/AST_serialization.h"

#include <xci/data/BinaryWriter.h>
#include <xci/data/BinaryReader.h>
#include <xci/data/Schema.h>
#include <xci/core/string.h>

#include <fstream>
#include <ranges>

namespace xci::script {


Module::~Module()
{
    #ifdef TRACE_REFCOUNT
    std::cout << "* in ~Module " << name() << std::endl;
    #endif
    for (auto& val : m_values) {
        val.decref();
    }
}


SymbolPointer Module::add_native_function(
        NameId name, TypeInfo&& param, TypeInfo&& retval, NativeDelegate native)
{
    Function fn {*this, symtab().add_child(name)};
    fn.signature().set_parameter(ti_normalize(std::move(param)));
    fn.signature().set_return_type(ti_normalize(std::move(retval)));
    fn.set_native(native);
    auto fn_idx = add_function(std::move(fn)).index;
    auto scope_idx = add_scope(Scope{*this, fn_idx, symtab().scope()});
    auto subscope_i = symtab().scope()->add_subscope(scope_idx);
    return symtab().add({name, Symbol::Function, subscope_i});
}


Index Module::import_module(NameId name)
{
    if (m_module_manager == nullptr)
        return no_index;

    m_modules.push_back(m_module_manager->import_module(name));
    const Index index = Index(m_modules.size() - 1);
    m_symtab.add({name, Symbol::Module, index});
    return index;
}


Index Module::add_imported_module(std::shared_ptr<Module> mod)
{
    m_modules.push_back(std::move(mod));
    const Index index = Index(m_modules.size() - 1);
    m_symtab.add({m_modules.back()->name(), Symbol::Module, index});
    return index;
}


Index Module::get_imported_module_index(Module* mod) const
{
    const auto it = std::ranges::find_if(m_modules,
            [mod](const std::shared_ptr<Module>& a){ return mod == a.get(); });
    if (it == m_modules.end())
        return no_index;
    return it - m_modules.begin();
}


Index Module::get_imported_module_index(NameId name) const
{
    auto it = std::ranges::find_if(m_modules,
                      [name](const std::shared_ptr<Module>& a){ return name == a->name(); });
    if (it == m_modules.end())
        return no_index;
    return it - m_modules.begin();
}


auto Module::add_function(Function&& fn) -> WeakFunctionId
{
    return m_functions.add(std::move(fn));
}


auto Module::find_function(NameId name) const -> WeakFunctionId
{
    const auto it = std::ranges::find_if(m_functions,
                           [name](const Function& fn) { return fn.name() == name; });
    if (it == m_functions.end())
        return decltype(m_functions)::not_found;
    return it.weak_index();
}


auto Module::add_scope(Scope&& scope) -> ScopeIdx
{
    assert(scope.parent() != nullptr);  // only main scope has no parent
    auto scope_idx = m_scopes.add(std::move(scope)).index;
    auto& rscope = get_scope(scope_idx);
    if (rscope.function_index() != no_index) {
        auto& symtab = rscope.module().get_function(rscope.function_index()).symtab();
        if (symtab.scope() == nullptr) {
            symtab.set_scope(&rscope);
        }
    }
    return scope_idx;
}


Index Module::add_value(TypedValue&& value)
{
    auto idx = find_value(value);
    if (idx != no_index) {
        value.decref();  // we don't save the new value -> release it
        return idx;
    }

    m_values.add(std::move(value));
    return Index(m_values.size() - 1);
}


Index Module::find_value(const TypedValue& value) const
{
    const auto it = std::ranges::find(m_values, value);
    if (it == m_values.end())
        return no_index;
    return it - m_values.begin();
}


Index Module::add_type(TypeInfo type_info)
{
    // lookup previous type (deduplicate)
    if (!type_info.has_unknown()) {
        auto idx = find_type(type_info);
        if (idx != no_index)
            return idx;
    }

    m_types.push_back(std::move(type_info));
    return Index(m_types.size() - 1);
}


void Module::update_type(Index index, TypeInfo type_info)
{
    // update possibly Unknown type with a concrete type
    assert(!type_info.has_unknown());
    assert(m_types[index] == type_info);  // it must differ only in Unknown fields
    m_types[index] = std::move(type_info);
}


Index Module::find_type(const TypeInfo& type_info) const
{
    assert(!type_info.has_generic());
    const auto it = std::ranges::find(m_types, type_info);
    if (it == m_types.end())
        return no_index;
    return it - m_types.begin();
}


auto Module::add_class(Class&& cls) -> WeakClassId
{
    return m_classes.add(std::move(cls));
}


auto Module::add_instance(Instance&& inst) -> WeakInstanceId
{
    return m_instances.add(std::move(inst));
}


SymbolTable& Module::symtab_by_qualified_name(std::string_view name)
{
    auto parts = core::split(name, "::");
    auto part_it = parts.begin();

    SymbolTable* symtab = nullptr;
    NameId part_name = intern(*part_it);
    if (part_name == m_symtab.name()) {
        // a symbol from this module
        symtab = &m_symtab;
    } else {
        // a symbol from an imported module
        for (const auto& module : m_modules)
            if (module->name() == part_name)
                symtab = &module->symtab();
        if (symtab == nullptr)
            throw unresolved_symbol(name);
    }

    while (++part_it != parts.end()) {
        symtab = symtab->find_child_by_name(intern(*part_it));
        if (symtab == nullptr)
            throw unresolved_symbol(name);
    }
    return *symtab;
}


void Module::add_spec_function(SymbolPointer gen_fn, Index spec_scope_idx)
{
    m_spec_functions.emplace(gen_fn, spec_scope_idx);
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


void Module::add_spec_instance(SymbolPointer gen_inst, Index spec_inst_idx)
{
    m_spec_instances.emplace(gen_inst, spec_inst_idx);
}


std::vector<Index> Module::get_spec_instances(SymbolPointer gen_inst)
{
    auto [beg, end] = m_spec_instances.equal_range(gen_inst);
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


// -----------------------------------------------------------------------------
// Module serialization

// Following static asserts help with development - error message readability
static_assert(xci::data::TypeWithSaveFunction<ast::Expression, xci::data::BinaryWriter>);


bool Module::save_to_file(const std::string& filename)
{
    std::ofstream f(filename, std::ios::binary);
    xci::data::BinaryWriter writer(f, true);
    writer(m_modules)(m_values)(m_symtab)(m_functions);  // NOLINT(clang-analyzer-core.uninitialized.UndefReturn) - https://github.com/boostorg/pfr/issues/91
    return !f.fail();
}


bool Module::write_schema_to_file(const std::string& filename)
{
    xci::data::Schema schema;
    schema  ("modules", m_modules)
            ("values", m_values)
            ("symtab", m_symtab)
            ("functions", m_functions);

    std::ofstream f(filename, std::ios::binary);
    xci::data::BinaryWriter writer(f, true);
    writer(schema);
    return !f.fail();
}


class ModuleLoader {
    ModuleManager& m_module_manager;
    std::vector<std::shared_ptr<Module>>& m_modules;

public:
    ModuleLoader(ModuleManager& module_manager, std::vector<std::shared_ptr<Module>>& modules)
        : m_module_manager(module_manager), m_modules(modules) {}

    template<class Archive>
    void load(Archive& ar) {
        std::string module_name;
        ar(module_name);
        m_modules.push_back(m_module_manager.import_module(module_name));
    }
};


bool Module::load_from_file(const std::string& filename)
{
    if (m_module_manager == nullptr)
        return false;
    // undo init()
    // FIXME: don't call init() from constructor, call it directly in REPL etc.
    m_functions.clear();
    m_scopes.clear();
    m_symtab.set_scope(nullptr);

    std::ifstream f(filename, std::ios::binary);

    struct ReaderContext {
        Module& module;
    };
    using ModuleReader = xci::data::BinaryReaderBase<ReaderContext>;
    ModuleReader reader(f, ReaderContext{*this});
    reader.repeated(m_modules, [this](std::vector<std::shared_ptr<Module>>& modules) {
        return ModuleLoader(*m_module_manager, modules);
    });
    reader(m_values)(m_symtab);
    reader.repeated(m_functions, [this](IndexedMap<Function>& functions) -> Function& {
        auto idx = m_functions.emplace(*this);
        return *m_functions.get(idx);
    });
    return !f.fail();
}


void Module::init()
{
    m_symtab.set_module(this);
    // create main function
    auto fn_idx = m_functions.emplace(*this, m_symtab).index;
    assert(fn_idx == 0);
    // create root scope
    auto scope_idx = m_scopes.emplace(*this, fn_idx, nullptr).index;
    assert(scope_idx == 0);
    m_symtab.set_scope(&m_scopes[scope_idx]);
}


} // namespace xci::script
