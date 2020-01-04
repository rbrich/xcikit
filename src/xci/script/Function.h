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

    const std::string& name() const { return m_symtab.name(); }

    // module containing this function
    Module& module() const { return m_module; }

    // symbol table with names used in function scope
    SymbolTable& symtab() const { return m_symtab; }

    // parameters
    void add_parameter(std::string name, TypeInfo&& type_info);
    bool has_parameters() const { return !m_signature->params.empty(); }
    const TypeInfo& get_parameter(Index idx) const { return m_signature->params[idx]; }
    void set_parameter(Index idx, TypeInfo&& ti) { m_signature->params[idx] = std::move(ti); }
    const std::vector<TypeInfo>& parameters() const { return m_signature->params; }
    size_t raw_size_of_parameters() const;
    size_t parameter_offset(Index idx) const;

    // function signature
    void set_signature(const std::shared_ptr<Signature>& newsig) { m_signature = newsig; }
    std::shared_ptr<Signature> signature_ptr() const { return m_signature; }
    Signature& signature() { return *m_signature; }
    const Signature& signature() const { return *m_signature; }

    // compiled function body
    Code& code() { return m_code; }
    const Code& code() const { return m_code; }

    // AST of function body
    bool has_ast() const { return m_ast != nullptr; }
    ast::Block* ast() const { return m_ast; }
    void set_ast(ast::Block* body) { m_ast = body; }

    // non-locals
    std::vector<TypeInfo> nonlocals() const;
    // return size of all nonlocals (whole closure) in bytes
    size_t raw_size_of_nonlocals() const;
    std::pair<size_t, TypeInfo> nonlocal_offset_and_type(Index idx) const;

    // true if this function is generic
    bool is_generic() const;

    // Special intrinsics function cannot contain any compiled code and is always inlined.
    // This counter helps to check no other code was generated.
    void add_intrinsic(uint8_t code) { m_intrinsics++; m_code.add_arg(code); }
    size_t intrinsics() const { return m_intrinsics; }
    bool has_intrinsics() const { return m_intrinsics > 0; }

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
    // Compiled function body
    Code m_code;
    // AST of function body (only for generic function)
    ast::Block* m_ast = nullptr;
    // Counter for instructions from intrinsics
    size_t m_intrinsics = 0;
};


} // namespace xci::script

#endif // include guard
