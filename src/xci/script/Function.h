// Function.h created on 2019-05-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_FUNCTION_H
#define XCI_SCRIPT_FUNCTION_H

#include "Code.h"
#include "ast/AST.h"
#include "SymbolTable.h"
#include "TypeInfo.h"
#include "NativeDelegate.h"
#include <map>
#include <string>
#include <variant>

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
    Code& code() { return std::get<CompiledBody>(m_body).code; }
    const Code& code() const { return std::get<CompiledBody>(m_body).code; }

    // Special intrinsics function cannot contain any compiled code and is always inlined.
    // This counter helps to check no other code was generated.
    void add_intrinsics(unsigned code_size) { std::get<CompiledBody>(m_body).intrinsics += code_size; }
    size_t intrinsics() const { return std::get<CompiledBody>(m_body).intrinsics; }
    bool has_intrinsics() const { return std::get<CompiledBody>(m_body).intrinsics > 0; }

    // Generic function: AST of function body
    struct GenericBody;
    GenericBody yank_generic_body() { return std::move(std::get<GenericBody>(m_body)); }
    const ast::Block& ast() const { return std::get<GenericBody>(m_body).ast(); }
    void set_ast(const ast::Block& body) { m_body = GenericBody{&body}; }
    bool is_ast_copied() const { return std::get<GenericBody>(m_body).ast_ref == nullptr; }
    void ensure_ast_copy() { std::get<GenericBody>(m_body).ensure_copy(); }

    // non-locals
    Index add_nonlocal(TypeInfo&& type_info);
    void set_nonlocal(Index idx, TypeInfo&& type_info);
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
    std::vector<TypeInfo> closure_types() const;

    // true if this function should be generic (i.e. signature contains a type variable)
    bool detect_generic() const { return m_signature->is_generic(); }
    bool has_generic_params() const { return m_signature->has_generic_params(); }

    // Kind of function body

    // function has compiled bytecode and will be called or inlined
    struct CompiledBody {
        bool operator==(const CompiledBody& rhs) const;

        // Compiled function body
        Code code;
        // Counter for code bytes from intrinsics
        unsigned intrinsics = 0;
    };

    // function is a template, signature contains type variables
    struct GenericBody {
        bool operator==(const GenericBody& rhs) const;

        // AST of function body - reference
        const ast::Block* ast_ref = nullptr;

        // frozen copy of AST (when ast_ref == nullptr)
        ast::Block ast_copy;

        // obtain read-only AST
        const ast::Block& ast() const { return ast_ref ? *ast_ref : ast_copy; }

        // copy AST if referenced
        void ensure_copy() {
            if (ast_ref) {
                ast_copy = ast::copy(*ast_ref);
                ast_ref = nullptr;
            }
        }
    };

    // function wraps native function (C++ binding)
    struct NativeBody {
        bool operator==(const NativeBody& rhs) const;

        NativeDelegate native;
    };

    void set_compiled() { m_body = CompiledBody{}; }

    void set_native(NativeDelegate native) { m_body = NativeBody{native}; }
    void call_native(Stack& stack) const { std::get<NativeBody>(m_body).native(stack); }

    bool is_undefined() const { return std::holds_alternative<std::monostate>(m_body); }
    bool is_compiled() const { return std::holds_alternative<CompiledBody>(m_body); }
    bool is_generic() const { return std::holds_alternative<GenericBody>(m_body); }
    bool is_native() const { return std::holds_alternative<NativeBody>(m_body); }

    enum class Kind {
        Undefined,  // not yet compiled
        Compiled,   // compiled into bytecode
        Generic,    // generic function in AST representation
        Native,     // wrapped native function (C++ binding)
    };
    Kind kind() const { return Kind(m_body.index()); }

    bool operator==(const Function& rhs) const;

private:
    Module& m_module;
    SymbolTable& m_symtab;
    // Function signature
    std::shared_ptr<Signature> m_signature;
    // Function body (depending on kind of function)
    std::variant<std::monostate, CompiledBody, GenericBody, NativeBody> m_body;
};


} // namespace xci::script

#endif // include guard
