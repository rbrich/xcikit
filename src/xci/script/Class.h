// Class.h created on 2019-09-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
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
    Class(Class&& rhs) noexcept;
    Class& operator =(Class&&) = delete;

    const std::string& name() const { return m_symtab.name(); }

    // symbol table associated with the class
    // (contains type_var and function prototypes)
    SymbolTable& symtab() const { return m_symtab; }

    void add_function(Index mod_fn_idx) { m_functions.push_back(mod_fn_idx); }
    Index get_function(size_t idx) const { return m_functions[idx]; }
    Index get_function_index(Index fn_idx) const;
    size_t num_functions() const { return m_functions.size(); }

private:
    SymbolTable& m_symtab;
    // functions in the class -> module function idx
    std::vector<Index> m_functions;
};


class Instance {
public:
    explicit Instance(Class& cls, SymbolTable& symtab);

    Class& class_() const { return m_class; }
    SymbolTable& symtab() const { return m_symtab; }

    // instantiation types
    void add_type(TypeInfo&& ti) { m_types.push_back(std::move(ti)); }
    void set_types(std::vector<TypeInfo> types) { m_types = std::move(types); }
    const std::vector<TypeInfo>& types() const { return m_types; }

    bool is_generic() const;

    // Functions
    struct FunctionInfo {
        Index index;  // function idx in module
        SymbolPointer symptr;  // SymbolPointer to associated symbol (for specialized function, this points to original generic one)
    };

    void set_function(Index cls_fn_idx, Index mod_fn_idx, SymbolPointer symptr);
    const FunctionInfo& get_function(Index cls_fn_idx) const { return m_functions[cls_fn_idx]; }
    Size num_functions() const { return Size(m_functions.size()); }

private:
    Class& m_class;
    SymbolTable& m_symtab;
    // instantiation types
    std::vector<TypeInfo> m_types;
    // functions in the instance - map of class function idx -> Index, (SymbolPointer)
    std::vector<FunctionInfo> m_functions;
};


} // namespace xci::script

#endif // include guard
