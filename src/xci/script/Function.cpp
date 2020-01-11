// Function.cpp created on 2019-05-30, part of XCI toolkit
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

#include "Function.h"
#include "Module.h"
#include "Error.h"
#include <range/v3/algorithm/any_of.hpp>
#include <utility>
#include <numeric>

namespace xci::script {

using namespace std;


Function::Function(Module& module, SymbolTable& symtab)
      : m_module(module), m_symtab(symtab),
        m_signature(std::make_shared<Signature>())
{
    symtab.set_function(this);
}


void Function::add_parameter(string name, TypeInfo&& type_info)
{
    m_symtab.add({std::move(name), Symbol::Parameter, parameters().size()});
    signature().add_parameter(std::move(type_info));
}


size_t Function::raw_size_of_parameters() const
{
    return std::accumulate(m_signature->params.begin(), m_signature->params.end(), 0,
               [](size_t init, const TypeInfo& ti) { return init + ti.size(); });
}


size_t Function::parameter_offset(Index idx) const
{
    size_t ofs = 0;
    for (const auto& ti : m_signature->params) {
        if (idx == 0)
            return ofs;
        ofs += ti.size();
        idx --;
    }
    assert(!"parameter index out of range");
    return 0;
}


void Function::add_nonlocal(TypeInfo&& type_info)
{
    signature().add_nonlocal(std::move(type_info));
}


size_t Function::raw_size_of_nonlocals() const
{
    return std::accumulate(nonlocals().begin(), nonlocals().end(), 0,
            [](size_t init, const TypeInfo& ti) { return init + ti.size(); });
}


std::pair<size_t, TypeInfo> Function::nonlocal_offset_and_type(Index idx) const
{
    size_t ofs = 0;
    for (const auto& ti : nonlocals()) {
        if (idx == 0)
            return {ofs, ti};
        ofs += ti.size();
        idx --;
    }
    assert(!"nonlocal index out of range");
    return {0, {}};
}


void Function::add_partial(TypeInfo&& type_info)
{
    signature().add_partial(std::move(type_info));
}


size_t Function::raw_size_of_partial() const
{
    return std::accumulate(partial().begin(), partial().end(), 0,
            [](size_t init, const TypeInfo& ti) { return init + ti.size(); });
}


std::vector<TypeInfo> Function::closure() const
{
    auto closure = nonlocals();
    std::copy(partial().cbegin(), partial().cend(), std::back_inserter(closure));
    return closure;
}


bool Function::is_generic() const
{
    return ranges::any_of(signature().params, [](const TypeInfo& type_info) {
        return type_info.type() == Type::Unknown;
    });
}


ostream& operator<<(ostream& os, const Function& v)
{
    os << v.signature() << endl;
    for (auto it = v.code().begin(); it != v.code().end(); it++) {
        os << ' ' << v.dump_instruction_at(it) << endl;
    }
    return os;
}


std::ostream& operator<<(std::ostream& os, Function::DumpInstruction f)
{
    auto inum = f.pos - f.func.code().begin();
    auto opcode = static_cast<Opcode>(*f.pos);
    os << right << setw(3) << inum << "  " << left << setw(20) << opcode;
    if (opcode >= Opcode::OneArgFirst && opcode <= Opcode::OneArgLast) {
        // 1 arg
        Index arg = *(++f.pos);
        os << static_cast<int>(arg);
        switch (opcode) {
            case Opcode::LoadStatic:
                os << " (" << f.func.module().get_value(arg) << ")";
                break;
            case Opcode::LoadFunction:
            case Opcode::Call0: {
                const auto& fn = f.func.module().get_function(arg);
                os << " (" << fn.symtab().name() << ' ' << fn.signature() << ")";
                break;
            }
            case Opcode::Call1: {
                const auto& fn = f.func.module().get_imported_module(0).get_function(arg);
                os << " (" << fn.symtab().name() << ' ' << fn.signature() << ")";
                break;
            }
            default:
                break;
        }
    }
    if (opcode >= Opcode::TwoArgFirst && opcode <= Opcode::TwoArgLast) {
        // 2 args
        Index arg1 = *(++f.pos);
        Index arg2 = *(++f.pos);
        os << static_cast<int>(arg1) << ' ' << static_cast<int>(arg2);
        switch (opcode) {
            case Opcode::Call: {
                const auto& fn = f.func.module().get_imported_module(arg1).get_function(arg2);
                os << " (" << fn.symtab().name() << ' ' << fn.signature() << ")";
                break;
            }
            default:
                break;
        }
    }
    return os;
}


bool Function::operator==(const Function& rhs) const
{
    return &m_module == &rhs.m_module &&
           &m_symtab == &rhs.m_symtab &&
           *m_signature == *rhs.m_signature &&
           m_code == rhs.m_code &&
           m_kind == rhs.m_kind && (
                m_kind == Kind::Native
                    ? m_native == rhs.m_native
                    : m_ast == rhs.m_ast
           );
}


} // namespace xci::script
