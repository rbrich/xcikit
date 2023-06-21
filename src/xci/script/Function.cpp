// Function.cpp created on 2019-05-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Function.h"
#include "Module.h"
#include <utility>
#include <numeric>

namespace xci::script {


Function::Function()
        : m_signature(std::make_shared<Signature>())
{}


Function::Function(Module& mod)
        : m_module(&mod),
          m_signature(std::make_shared<Signature>())
{}


Function::Function(Module& mod, SymbolTable& symtab)
        : m_module(&mod), m_symtab(&symtab),
          m_signature(std::make_shared<Signature>())
{
    m_symtab->set_function(this);
}


Function::Function(Function&& rhs) noexcept
        : m_module(rhs.m_module), m_symtab(rhs.m_symtab),
          m_signature(std::move(rhs.m_signature)),
          m_body(std::move(rhs.m_body))
{
    m_symtab->set_function(this);
}


const TypeInfo& Function::parameter(Index idx) const
{
    if (idx == no_index)
        return m_signature->param_type;
    if (m_signature->param_type.is_struct())
        return m_signature->param_type.struct_items()[idx].second;
    assert(m_signature->param_type.is_tuple());
    return m_signature->param_type.subtypes()[idx];
}


size_t Function::parameter_offset(Index idx) const
{
    if (idx == no_index)
        return 0;
    assert(m_signature->param_type.is_struct_or_tuple());
    size_t ofs = 0;
    for (const auto& ti : m_signature->param_type.struct_or_tuple_subtypes()) {
        if (idx == 0)
            return ofs;
        ofs += ti.size();
        idx --;
    }
    assert(!"parameter index out of range");
    return 0;
}


size_t Function::raw_size_of_nonlocals() const
{
    return std::accumulate(nonlocals().begin(), nonlocals().end(), size_t(0),
            [](size_t init, const TypeInfo& ti) { return init + ti.size(); });
}


size_t Function::num_type_params() const
{
    return std::count_if(symtab().begin(), symtab().end(),
             [](const Symbol& sym) {
                 return sym.type() == Symbol::TypeVar && sym.name().front() != '$';
             });
}


bool Function::operator==(const Function& rhs) const
{
    return m_module == rhs.m_module &&
           &m_symtab == &rhs.m_symtab &&
           *m_signature == *rhs.m_signature &&
           m_body == rhs.m_body;
}


void Function::set_symtab_by_qualified_name(std::string_view name)
{
    assert(m_module != nullptr);
    m_symtab = &m_module->symtab_by_qualified_name(name);
}


void Function::copy_body(const Function& src)
{
    return std::visit([this](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, GenericBody>) {
            m_body = GenericBody{nullptr, v.ast().make_copy()};
        } else {
            m_body = v;
        }
    }, src.m_body);
}


bool Function::CompiledBody::operator==(const Function::CompiledBody& rhs) const
{
    return code == rhs.code;
}


bool Function::NativeBody::operator==(const Function::NativeBody& rhs) const
{
    return native == rhs.native;
}


// -----------------------------------------------------------------------------


Scope::Scope(Module& module, Index function_idx, Scope* parent_scope)
    : m_module(&module), m_function(function_idx), m_parent_scope(parent_scope)
{
}


Function& Scope::function() const
{
    return m_module->get_function(m_function);
}


Index Scope::add_subscope(Index scope_idx)
{
    auto it = std::find(m_subscopes.begin(), m_subscopes.end(), scope_idx);
    if (it != m_subscopes.end()) {
        // Return previously added function index
        return Index(it - m_subscopes.begin());
    }

    m_subscopes.push_back(scope_idx);
    return Index(m_subscopes.size() - 1);
}


void Scope::copy_subscopes(const Scope& from)
{
    for (const Index scope_idx : from.m_subscopes) {
        auto& orig = module().get_scope(scope_idx);
        Scope sub(orig.module(), orig.function_index(), this);
        auto sub_idx = module().add_scope(std::move(sub));
        module().get_scope(sub_idx).copy_subscopes(orig);
        add_subscope(sub_idx);
    }
}


Index Scope::get_index_of_subscope(Index mod_scope_idx) const
{
    auto it = std::find(m_subscopes.begin(), m_subscopes.end(), mod_scope_idx);
    return Index(it - m_subscopes.begin());
}


Scope& Scope::get_subscope(Index idx) const
{
    return module().get_scope(m_subscopes[idx]);
}


const Scope* Scope::find_parent_scope(const SymbolTable* symtab) const
{
    const Scope* scope = this;
    while (scope->has_function() && &scope->function().symtab() != symtab) {
        scope = scope->parent();
        if (scope == nullptr)
            return nullptr;
    }
    return scope->has_function() ? scope : nullptr;
}


void Scope::add_nonlocal(Index index)
{
    m_nonlocals.emplace_back(Nonlocal{index});
}


void Scope::add_nonlocal(Index index, TypeInfo ti, Index fn_scope_idx)
{
    auto& sig = function().signature();
    assert(m_nonlocals.size() <= sig.nonlocals.size());
    for (unsigned i = 0; i != m_nonlocals.size(); ++i) {
        if (m_nonlocals[i].index == index && sig.nonlocals[i] == ti) {
            assert(m_nonlocals[i].fn_scope_idx == fn_scope_idx);
            return;  // already exists
        }
    }
    m_nonlocals.emplace_back(Nonlocal{index, fn_scope_idx});
    if (ti.is_callable() && ti.signature_ptr() == function().signature_ptr()) {
        // copy if the target signature is same object as ti.signature
        ti = TypeInfo(std::make_shared<Signature>(ti.signature()));
    }
    auto new_i = m_nonlocals.size() - 1;
    if (new_i < sig.nonlocals.size()) {
        assert(sig.nonlocals[new_i] == ti);  // orig can be of type Unknown, but not different type
        sig.nonlocals[new_i] = ti;
        return;
    }
    sig.add_nonlocal(std::move(ti));
}


size_t Scope::nonlocal_raw_offset(Index index, const TypeInfo& ti) const
{
    size_t ofs = 0;
    auto& sig = function().signature();
    assert(m_nonlocals.size() == sig.nonlocals.size());
    for (unsigned i = 0; i != m_nonlocals.size(); ++i) {
        if (m_nonlocals[i].index == index && sig.nonlocals[i] == ti)
            return ofs;
        ofs += sig.nonlocals[i].size();
    }
    assert(!"nonlocal index out of range");
    return 0;
}


bool Scope::has_unresolved_type_params() const
{
    SymbolTable& symtab = function().symtab();
    return std::any_of(symtab.begin(), symtab.end(), [this, &symtab](const Symbol& sym) {
        return sym.type() == Symbol::TypeVar && sym.name().front() != '$' &&
               m_type_args.get(symtab.find(sym)).is_unknown();
    });
}


} // namespace xci::script
