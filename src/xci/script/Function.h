// Function.h created on 2019-05-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_FUNCTION_H
#define XCI_SCRIPT_FUNCTION_H

#include "Code.h"
#include "CodeAssembly.h"
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
    Function();  // only for deserialization!
    explicit Function(Module& mod);  // only for deserialization!
    explicit Function(Module& mod, SymbolTable& symtab);
    Function(Function&& rhs) noexcept;
    Function& operator =(Function&&) = delete;

    bool operator==(const Function& rhs) const;

    NameId name() const { return m_symtab->name(); }
    std::string qualified_name() const { return m_symtab == nullptr ? "" : m_symtab->qualified_name(); }

    // module containing this function
    Module& module() const { return *m_module; }

    // symbol table with names used in function scope
    SymbolTable& symtab() const { return *m_symtab; }

    // parameters
    bool has_nonvoid_parameter() const { return m_signature->has_nonvoid_param(); }
    const TypeInfo& parameter() const { return m_signature->param_type; }
    const TypeInfo& parameter(Index idx) const;
    size_t raw_size_of_parameter() const { return parameter().size(); }
    size_t parameter_offset(Index idx) const;

    // function signature
    void set_signature(const SignaturePtr& sig) { m_signature = sig; }
    SignaturePtr signature_ptr() const { return m_signature; }
    Signature& signature() { return *m_signature; }
    const Signature& signature() const { return *m_signature; }

    // effective return type
    TypeInfo effective_return_type() const { return m_signature->return_type.effective_type(); }

    // compiled function body
    CodeAssembly& asm_code() { return std::get<AssemblyBody>(m_body).code; }
    const CodeAssembly& asm_code() const { return std::get<AssemblyBody>(m_body).code; }
    Code& bytecode() { return std::get<BytecodeBody>(m_body).code; }
    const Code& bytecode() const { return std::get<BytecodeBody>(m_body).code; }
    void assembly_to_bytecode();

    // Special intrinsics function cannot contain any compiled code and is always inlined.
    // This counter helps to check no other code was generated.
    void add_intrinsics() { std::get<AssemblyBody>(m_body).intrinsics += 1; }
    size_t intrinsics() const { return std::get<AssemblyBody>(m_body).intrinsics; }
    bool has_intrinsics() const { return std::get<AssemblyBody>(m_body).intrinsics > 0; }

    // Generic function: AST of function body
    struct GenericBody;
    GenericBody yank_generic_body() { return std::move(std::get<GenericBody>(m_body)); }
    const ast::Expression& ast() const { return std::get<GenericBody>(m_body).ast(); }
    ast::Expression& ast() { return std::get<GenericBody>(m_body).ast(); }
    void set_ast(ast::Expression& expr) { m_body = GenericBody{&expr}; }
    bool is_ast_copied() const { return std::get<GenericBody>(m_body).ast_ref == nullptr; }
    void ensure_ast_copy() { std::get<GenericBody>(m_body).ensure_copy(); }

    // non-locals (closure)
    bool has_nonlocals() const { return !m_signature->nonlocals.empty(); }
    const std::vector<TypeInfo>& nonlocals() const { return m_signature->nonlocals; }
    size_t raw_size_of_nonlocals() const;  // size of all nonlocals in bytes

    // true if this function should be generic (i.e. signature contains a type variable)
    bool has_any_generic() const { return m_signature->has_any_generic(); }
    bool has_generic_param() const { return m_signature->param_type.has_generic(); }
    bool has_generic_return_type() const { return m_signature->return_type.has_generic(); }
    size_t num_type_params() const;

    // Kind of function body

    // function has compiled bytecode
    struct BytecodeBody {
        bool operator==(const BytecodeBody& rhs) const { return code == rhs.code; }

        template<class Archive>
        void serialize(Archive& ar) {
            ar("code", code);
        }

        Code code;
    };

    // function has intermediate relocatable compiled bytecode
    struct AssemblyBody {
        bool operator==(const AssemblyBody& rhs) const { return code == rhs.code; }

        template<class Archive>
        void save(Archive& ar) const {
            ar("code_asm", code);
        }

        template<class Archive>
        void load(Archive& ar) {
            throw std::runtime_error("CodeAssembly function cannot be deserialized");
        }

        CodeAssembly code;
        unsigned intrinsics = 0;    // Counter for code bytes from intrinsics
    };

    // function is not yet compiled or generic
    struct GenericBody {
        bool operator==(const GenericBody& rhs) const { return false; }

        // AST of function body - as a reference
        ast::Expression* ast_ref = nullptr;

        // frozen copy of AST (when ast_ref == nullptr)
        std::unique_ptr<ast::Expression> ast_copy;

        // obtain read-only AST
        const ast::Expression& ast() const { return ast_ref ? *ast_ref : *ast_copy; }
        ast::Expression& ast() { return ast_ref ? *ast_ref : *ast_copy; }

        // copy AST if referenced
        void ensure_copy() {
            if (ast_ref) {
                ast_copy = ast_ref->make_copy();
                ast_ref = nullptr;
            }
        }

        template<class Archive>
        void save(Archive& ar) const {
            ar("ast", ast());
        }

        template<class Archive>
        void load(Archive& ar) {
            //ar(ast_copy);
        }
    };

    // function wraps native function (C++ binding)
    struct NativeBody {
        bool operator==(const NativeBody& rhs) const;

        template<class Archive>
        void save(Archive& ar) const {
            ar("native", uintptr_t(&native));
        }

        template<class Archive>
        void load(Archive& ar) {
            throw std::runtime_error("Native function cannot be deserialized");
        }

        NativeDelegate native;
    };

    void set_undefined() { m_body = std::monostate{}; }
    void set_assembly() { m_body = AssemblyBody{}; m_compile = false; }
    void set_bytecode() { m_body = BytecodeBody{}; }

    void set_native(NativeDelegate native) { m_body = NativeBody{native}; }
    void call_native(Stack& stack) const { std::get<NativeBody>(m_body).native(stack); }

    bool is_undefined() const { return std::holds_alternative<std::monostate>(m_body); }
    bool is_bytecode() const { return std::holds_alternative<BytecodeBody>(m_body); }
    bool is_assembly() const { return std::holds_alternative<AssemblyBody>(m_body); }
    bool is_generic() const { return std::holds_alternative<GenericBody>(m_body); }
    bool is_native() const { return std::holds_alternative<NativeBody>(m_body); }

    void copy_body(const Function& src);

    void set_expression(bool is_expr = true) { m_expression = is_expr; }
    bool is_expression() const { return m_expression; }

    void set_specialized() { m_specialized = true; }
    bool is_specialized() const { return m_specialized; }

    void set_compile(bool compile = true) { m_compile = compile; }
    bool has_compile() const { return m_compile; }

    void set_nonlocals_resolved(bool nonlocals_resolved = true) { m_nonlocals_resolved = nonlocals_resolved; }
    bool has_nonlocals_resolved() const { return m_nonlocals_resolved; }

    enum class Kind {
        Undefined,  // not yet compiled
        Bytecode,   // compiled into bytecode
        Assembly,   // compiled into assembly code
        Generic,    // generic function in AST representation
        Native,     // wrapped native function (C++ binding)
    };
    Kind kind() const { return Kind(m_body.index()); }

    template<class Archive>
    void save(Archive& ar) const {
        ar("name", qualified_name());
        ar("signature", m_signature);
        ar("body", m_body);
    }

    template<class Archive>
    void load(Archive& ar) {
        std::string qualified_name;
        ar(qualified_name);
        ar(m_signature);
        ar(m_body);
        set_symtab_by_qualified_name(qualified_name);
        m_symtab->set_function(this);
    }

private:
    void set_symtab_by_qualified_name(std::string_view name);

    Module* m_module = nullptr;
    SymbolTable* m_symtab = nullptr;
    // function signature
    SignaturePtr m_signature;
    // function body (depending on kind of function)
    std::variant<std::monostate, BytecodeBody, AssemblyBody, GenericBody, NativeBody> m_body;
    // flags
    bool m_expression : 1 = false;  // doesn't have its own parameters, but can alias something with parameters
    bool m_specialized : 1 = false;
    bool m_compile : 1 = false;
    bool m_nonlocals_resolved : 1 = false;
};


class TypeArgs {
public:
    using map_type = std::map<SymbolPointer, TypeInfo>;
    using iterator = typename map_type::iterator;
    using const_iterator = typename map_type::const_iterator;

    // Returns Unknown when not contained in the map
    TypeInfo get(SymbolPointer sym) const {
        if (!sym)
            return TypeInfo{};
        auto it = m_type_args.find(sym);
        if (it == m_type_args.end())
            return TypeInfo{};
        return it->second;
    }

    std::pair<iterator, bool> set(SymbolPointer sym, TypeInfo&& ti) {
        if (ti.is_unknown() && (!ti.is_generic() || ti.generic_var() == sym))
            return { m_type_args.end(), true };  // "inserted" Unknown to nowhere
        ti.set_key({});
        return m_type_args.try_emplace(sym, std::move(ti));
    }

    std::pair<iterator, bool> set(SymbolPointer sym, const TypeInfo& ti) {
        if (ti.is_unknown() && (!ti.is_generic() || ti.generic_var() == sym))
            return { m_type_args.end(), true };  // "inserted" Unknown to nowhere
        TypeInfo r = ti;
        r.set_key({});
        return m_type_args.try_emplace(sym, std::move(r));
    }

    void add_from(const TypeArgs& other) {
        for (const auto& it : other.m_type_args) {
            set(it.first, it.second);
        }
    }

    // Must not query Unknown or a symbol that is not in the map
    TypeInfo& operator[] (SymbolPointer sym) {
        return m_type_args[sym];
    }

    bool empty() const noexcept { return m_type_args.empty(); }
    const_iterator begin() const noexcept { return m_type_args.begin(); }
    const_iterator end() const noexcept { return m_type_args.end(); }

private:
    std::map<SymbolPointer, TypeInfo> m_type_args;
};


class Scope {
public:
    Scope() = default;
    explicit Scope(Module& module, Index function_idx, Scope* parent_scope);

    Module& module() const { return *m_module; }
    void set_module(Module& mod) { m_module = &mod; }

    bool has_function() const { return m_function != no_index; }
    Function& function() const;
    void set_function_index(Index fn_idx) { m_function = fn_idx; }
    Index function_index() const { return m_function; }

    Scope* parent() const { return m_parent_scope; }

    // Nested functions
    Index add_subscope(Index scope_idx);
    void copy_subscopes(const Scope& from);
    Index get_subscope_index(Index idx) const { return m_subscopes[idx]; }
    void set_subscope_index(Index idx, Index scope_idx) { m_subscopes[idx] = scope_idx; }
    Index get_index_of_subscope(Index mod_scope_idx) const;
    Scope& get_subscope(Index idx) const;
    Size num_subscopes() const { return Size(m_subscopes.size()); }
    bool has_subscopes() const noexcept { return !m_subscopes.empty(); }
    const std::vector<Index>& subscopes() const { return m_subscopes; }

    // SymbolTable mapping (a SymbolTable may map to multiple scope hierarchies)
    // Find a scope (this or parent of this) matching the `symtab`
    const Scope* find_parent_scope(const SymbolTable* symtab) const;

    // Non-local values needed by nested functions, closures
    // The nonlocal may reference a parent Nonlocal symbol - in that case, the value must be captured
    // by parent scope, so this scope can use it.
    struct Nonlocal {
        Index index;  // index() from symbol of type Symbol::Nonlocal in this function's SymbolTable
        Index fn_scope_idx;  // scope index of resolved overloaded Function (target of the nonlocal)
    };
    void add_nonlocal(Index index);
    void add_nonlocal(Index index, TypeInfo ti, Index fn_scope_idx = no_index);
    bool has_nonlocals() const noexcept { return !m_nonlocals.empty(); }
    const std::vector<Nonlocal>& nonlocals() const { return m_nonlocals; }
    size_t nonlocal_raw_offset(Index index, const TypeInfo& ti) const;

    const TypeArgs& type_args() const { return m_type_args; }
    TypeArgs& type_args() { return m_type_args; }
    bool has_type_args() const noexcept { return !m_type_args.empty(); }
    bool has_unresolved_type_params() const;

private:
    Module* m_module = nullptr;
    Index m_function = no_index;  // function index in module
    Scope* m_parent_scope = nullptr;  // matches `m_symtab.parent()`, but can be a specialized function, while symtab is only lexical
    std::vector<Index> m_subscopes;  // nested scopes (Index into module scopes)
    std::vector<Nonlocal> m_nonlocals;
    TypeArgs m_type_args;  // resolved type variables or explicit type args
};


} // namespace xci::script

#endif // include guard
