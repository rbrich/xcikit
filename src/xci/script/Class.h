// Class.h created on 2019-09-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_CLASS_H
#define XCI_SCRIPT_CLASS_H

#include "TypeInfo.h"
#include "SymbolTable.h"

namespace xci::script {

class Function;


class Class {
public:
    explicit Class(SymbolTable& symtab);

    const std::string& name() const { return m_symtab.name(); }

    // symbol table associated with the class
    // (contains type_var and function prototypes)
    SymbolTable& symtab() const { return m_symtab; }

    Index add_function_type(TypeInfo&& type_info);
    const TypeInfo& get_function_type(size_t idx) const { return m_functions[idx]; }
    size_t num_functions() const { return m_functions.size(); }

private:
    SymbolTable& m_symtab;
    // functions in the class
    std::vector<TypeInfo> m_functions;
};


class Instance {
public:
    explicit Instance(Class& cls, SymbolTable& symtab);

    SymbolTable& symtab() const { return m_symtab; }

    // instantiation type
    void set_type(TypeInfo&& ti) { m_type = std::move(ti); }
    const TypeInfo& type() const { return m_type; }

    Class& class_() const { return m_class; }

    // Functions
    void set_function(Index cls_fn_idx, Index mod_fn_idx);
    Index get_function(size_t idx) const { return m_functions[idx]; }
    size_t num_functions() const { return m_functions.size(); }

private:
    Class& m_class;
    SymbolTable& m_symtab;
    // instantiation type
    TypeInfo m_type;
    // functions in the instance - map of class function idx -> module function idx
    std::vector<Index> m_functions;
};


} // namespace xci::script

#endif // include guard
