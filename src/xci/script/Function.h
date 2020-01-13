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
#include "SymbolTable.h"
#include "TypeInfo.h"
#include "NativeDelegate.h"
#include <map>
#include <string>

namespace xci::script {

class Module;
class Stack;

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
    const TypeInfo& parameter(Index idx) const { return m_signature->params[idx]; }
    const std::vector<TypeInfo>& parameters() const { return m_signature->params; }
    size_t raw_size_of_parameters() const;
    size_t parameter_offset(Index idx) const;

    // function signature
    void set_signature(const std::shared_ptr<Signature>& newsig) { m_signature = newsig; }
    std::shared_ptr<Signature> signature_ptr() const { return m_signature; }
    Signature& signature() { return *m_signature; }
    const Signature& signature() const { return *m_signature; }

    // effective return type
    TypeInfo effective_return_type() const { return m_signature->return_type.effective_type(); }

    // compiled function body
    Code& code() { return m_code; }
    const Code& code() const { return m_code; }

    // AST of function body
    bool has_ast() const { assert(m_kind != Kind::Native); return m_ast != nullptr; }
    ast::Block* ast() const { assert(m_kind != Kind::Native); return m_ast; }
    void set_ast(ast::Block* body) { assert(m_kind != Kind::Native); m_ast = body; }

    // non-locals
    void add_nonlocal(TypeInfo&& type_info);
    bool has_nonlocals() const { return !m_signature->nonlocals.empty(); }
    const std::vector<TypeInfo>& nonlocals() const { return m_signature->nonlocals; }
    size_t raw_size_of_nonlocals() const;  // size of all nonlocals in bytes
    std::pair<size_t, TypeInfo> nonlocal_offset_and_type(Index idx) const;

    // partial call (bound args)
    void add_partial(TypeInfo&& type_info);
    const std::vector<TypeInfo>& partial() const { return m_signature->partial; }
    size_t raw_size_of_partial() const;

    // whole closure = nonlocals + partial
    size_t raw_size_of_closure() const { return raw_size_of_nonlocals() + raw_size_of_partial(); }
    size_t closure_size() const { return nonlocals().size() + partial().size(); }
    std::vector<TypeInfo> closure() const;

    // true if this function is generic (i.e. signature contains a type variable)
    bool is_generic() const;

    // Special intrinsics function cannot contain any compiled code and is always inlined.
    // This counter helps to check no other code was generated.
    void add_intrinsic(uint8_t code) { m_intrinsics++;
        m_code.add(code); }
    size_t intrinsics() const { return m_intrinsics; }
    bool has_intrinsics() const { return m_intrinsics > 0; }

    void set_native(NativeDelegate native) { m_kind = Kind::Native; m_native = native; }
    void call_native(Stack& stack) const { assert(m_kind == Kind::Native); m_native(stack); }

    // Flags
    enum class Kind {
        Normal,     // function has code and will be called
        Inline,     // function will be inlined at call site
        Generic,    // function is a template, signature contains type variables
        Native,     // function wraps native function (C++ binding)
    };
    void set_kind(Kind kind) { m_kind = kind; }
    Kind kind() const { return m_kind; }
    bool is_native() const { return m_kind == Kind::Native; }

    bool operator==(const Function& rhs) const;

private:
    Module& m_module;
    SymbolTable& m_symtab;
    // Function signature
    std::shared_ptr<Signature> m_signature;
    // Compiled function body
    Code m_code;
    union {
        // AST of function body (only for generic function)
        ast::Block* m_ast = nullptr;
        NativeDelegate m_native;
    };
    // Counter for instructions from intrinsics
    size_t m_intrinsics = 0;
    // Kind of function
    Kind m_kind = Kind::Normal;
};


} // namespace xci::script

#endif // include guard
