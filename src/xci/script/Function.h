// Function.h created on 2019-05-30, part of XCI toolkit
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

#ifndef XCI_SCRIPT_FUNCTION_H
#define XCI_SCRIPT_FUNCTION_H

#include "Value.h"
#include "Code.h"
#include "AST.h"
#include "Module.h"
#include "SymbolTable.h"
#include <map>
#include <string>

namespace xci::script {


// Scope of names and values
//
// Calling convention:
// - function args (in reversed order, first on top)
// - nonlocals (in reversed order, first on top)
// - base pointer points above args and nonlocals (first local var)
// - local variables


// Function bytecode:
// - caller:
//      - pushes args on stack in reversed order (first arg on top)
// - callee (the function itself):
//      - local definition are pushed on stack (last on top)
//      - operations referencing local defs or params use
//        COPY opcode to retrieve value from stack and push it on top

class Function {
public:
    explicit Function(Module& module, SymbolTable& symtab);

    // module containing this function
    Module& module() const { return m_module; }

    // symbol table with names used in this block
    SymbolTable& symtab() const { return m_symtab; }

    void add_parameter(std::string name, TypeInfo&& type_info);
    bool has_parameters() const { return !m_signature->params.empty(); }
    const TypeInfo& get_parameter(Index idx) const { return m_signature->params[idx]; }
    void set_parameter(Index idx, TypeInfo&& ti) { m_signature->params[idx] = std::move(ti); }
    const std::vector<TypeInfo>& parameters() const { return m_signature->params; }
    size_t raw_size_of_parameters() const;
    size_t parameter_offset(Index idx) const;

    std::shared_ptr<Signature> signature_ptr() const { return m_signature; }
    Signature& signature() { return *m_signature; }
    const Signature& signature() const { return *m_signature; }

    Code& code() { return m_code; }
    const Code& code() const { return m_code; }

    Index add_value(TypeInfo&& type_info);
    const TypeInfo& get_value(Index idx) const { return m_values[idx]; }
    void set_value(Index idx, TypeInfo&& ti) { m_values[idx] = std::move(ti); }
    std::vector<TypeInfo>& values() { return m_values; }
    const std::vector<TypeInfo>& values() const { return m_values; }
    size_t raw_size_of_values() const;
    size_t value_offset(Index idx) const;

    std::vector<TypeInfo> nonlocals() const;
    // return size of all nonlocals (whole closure) in bytes
    size_t raw_size_of_nonlocals() const;
    std::pair<size_t, TypeInfo> nonlocal_offset_and_type(Index idx) const;

    // true if this function is generic
    bool is_generic() const;

    // return new function with applied args (args are drained, left empty)
    //Function partial_call(std::vector<Value>& args) const;

    bool operator==(const Function& rhs) const;

    friend std::ostream& operator<<(std::ostream& os, const Function& v);

    struct DumpInstruction { const Function& func; Code::const_iterator& pos; };
    DumpInstruction dump_instruction_at(Code::const_iterator& pos) const { return {*this, pos}; }
    friend std::ostream& operator<<(std::ostream& os, DumpInstruction f);

private:
    Module& m_module;
    SymbolTable& m_symtab;
    // Function signature
    std::shared_ptr<Signature> m_signature;
    // Local definitions
    std::vector<TypeInfo> m_values;
    // Function code
    Code m_code;
};


} // namespace xci::script

#endif // include guard
