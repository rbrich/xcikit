// Class.cpp created on 2019-09-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Class.h"
#include "Function.h"

#include <range/v3/algorithm.hpp>

namespace xci::script {


Class::Class(SymbolTable& symtab)
        : m_symtab(symtab)
{
    m_symtab.set_class(this);
}


Class::Class(Class&& rhs) noexcept
        : m_symtab(rhs.m_symtab), m_scopes(std::move(rhs.m_scopes))
{
    m_symtab.set_class(this);
}


Index Class::get_index_of_function(Index mod_scope_idx) const
{
    auto it = std::find(m_scopes.begin(), m_scopes.end(), mod_scope_idx);
    return Index(it - m_scopes.begin());
}


Instance::Instance(Class& cls, SymbolTable& symtab)
        : m_class(cls), m_symtab(symtab)
{
    m_symtab.set_class(&cls);
}


bool Instance::is_generic() const
{
    return ranges::any_of(m_types, [](const TypeInfo& t) { return t.has_generic(); });
}


void Instance::set_function(Index cls_fn_idx, Index mod_scope_idx, SymbolPointer symptr)
{
    m_functions.resize(m_class.num_function_scopes());
    m_functions[cls_fn_idx] = FunctionInfo{mod_scope_idx, symptr};
}


} // namespace xci::script
