// Class.cpp created on 2019-09-11, part of XCI toolkit
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
