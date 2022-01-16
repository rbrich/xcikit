// Class.cpp created on 2019-09-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Class.h"
#include "Function.h"

#include <range/v3/algorithm.hpp>

namespace xci::script {

using std::move;
using ranges::any_of;


Class::Class(SymbolTable& symtab)
        : m_symtab(symtab)
{
    m_symtab.set_class(this);
}


Class::Class(Class&& rhs)
        : m_symtab(rhs.m_symtab), m_functions(std::move(rhs.m_functions))
{
    m_symtab.set_class(this);
}


Index Class::get_function_index(Index fn_idx) const
{
    auto it = std::find(m_functions.begin(), m_functions.end(), fn_idx);
    return Index(it - m_functions.begin());
}


Instance::Instance(Class& cls, SymbolTable& symtab)
        : m_class(cls), m_symtab(symtab)
{
    m_symtab.set_class(&cls);
}


bool Instance::is_generic() const
{
    return ranges::any_of(m_types, [](const TypeInfo& t) { return t.is_generic(); });
}


void Instance::set_function(Index cls_fn_idx, Index mod_fn_idx, SymbolPointer symptr)
{
    m_functions.resize(m_class.num_functions());
    m_functions[cls_fn_idx] = FunctionInfo{mod_fn_idx, symptr};
}


} // namespace xci::script
