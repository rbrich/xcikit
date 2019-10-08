// Class.h created on 2019-09-11, part of XCI toolkit
// Copyright 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef XCI_SCRIPT_CLASS_H
#define XCI_SCRIPT_CLASS_H

#include "TypeInfo.h"
#include "SymbolTable.h"

namespace xci::script {

class Function;


class Class {
public:
    explicit Class(Module& module, SymbolTable& symtab);

    // symbol table associated with the class
    // (contains type_var and function prototypes)
    SymbolTable& symtab() const { return m_symtab; }

    Index add_function_type(TypeInfo&& type_info);
    const TypeInfo& get_function_type(size_t idx) const { return m_functions[idx]; }

private:
    Module& m_module;
    SymbolTable& m_symtab;
    // functions in the class
    std::vector<TypeInfo> m_functions;
};


class Instance {
public:
    explicit Instance(Class& cls, SymbolTable& symtab);

    SymbolTable& symtab() const { return m_symtab; }

    void set_type_inst(TypeInfo&& ti) { m_type_inst = std::move(ti); }
    const TypeInfo& type_inst() const { return m_type_inst; }

    Class& class_() const { return m_class; }

    // Functions
    Index add_function(std::unique_ptr<Function>&& fn);
    Function& get_function(size_t idx) const { return *m_functions[idx]; }

private:
    Class& m_class;
    SymbolTable& m_symtab;
    // instantiation type
    TypeInfo m_type_inst;
    // functions in the instance
    std::vector<std::unique_ptr<Function>> m_functions;
};


} // namespace xci::script

#endif // include guard
