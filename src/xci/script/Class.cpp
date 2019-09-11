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

using namespace std;


Class::Class(Module& module, SymbolTable& symtab)
        : m_module(module), m_symtab(symtab)
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


Index Instance::add_function(std::unique_ptr<Function>&& fn)
{
    m_functions.push_back(move(fn));
    return m_functions.size() - 1;
}


} // namespace xci::script
