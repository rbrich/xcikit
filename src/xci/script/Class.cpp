// Class.cpp created on 2019-09-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Class.h"
#include "Function.h"

namespace xci::script {

using std::move;


Class::Class(SymbolTable& symtab)
        : m_symtab(symtab)
{
    symtab.set_class(this);
}


Index Class::add_function_type(TypeInfo&& type_info)
{
    m_functions.push_back(move(type_info));
    return m_functions.size() - 1;
}


Instance::Instance(Class& cls, SymbolTable& symtab)
        : m_class(cls), m_symtab(symtab)
{
    symtab.set_class(&cls);
}


void Instance::set_function(Index cls_fn_idx, Index mod_fn_idx)
{
    m_functions.resize(m_class.num_functions(), no_index);
    m_functions[cls_fn_idx] = mod_fn_idx;
}


} // namespace xci::script
