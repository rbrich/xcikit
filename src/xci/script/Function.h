// Function.h created on 2019-05-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
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
    Function();  // only for deserialization!
    explicit Function(Module& module);  // only for deserialization!
    explicit Function(Module& module, SymbolTable& symtab);
    Function(Function&& rhs) noexcept;
    Function& operator =(Function&&) = delete;

    bool operator==(const Function& rhs) const;

    const std::string& name() const { return m_symtab->name(); }
    std::string qualified_name() const { return m_symtab == nullptr ? "" : m_symtab->qualified_name(); }

    // module containing this function
    Module& module() const { return *m_module; }

    // symbol table with names used in function scope
    SymbolTable& symtab() const { return *m_symtab; }

    // parameters
    void add_parameter(std::string name, TypeInfo&& type_info);
    bool has_parameters() const { return !m_signature->params.empty(); }
    const TypeInfo& parameter(Index idx) const { return m_signature->params[idx]; }
    const std::vector<TypeInfo>& parameters() const { return m_signature->params; }
    size_t raw_size_of_parameters() const;
    size_t parameter_offset(Index idx) const;

    // function signature
    void set_signature(const std::shared_ptr<Signature>& sig) { m_signature = sig; }
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
    void add_intrinsics(size_t code_size) { std::get<CompiledBody>(m_body).intrinsics += (unsigned int) code_size; }
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
    bool has_nonlocals() const { return !m_signature->nonlocals.empty(); }
    const std::vector<TypeInfo>& nonlocals() const { return m_signature->nonlocals; }
    size_t raw_size_of_nonlocals() const;  // size of all nonlocals in bytes

    // partial call (bound args)
    void add_partial(TypeInfo&& type_info);
    const std::vector<TypeInfo>& partial() const { return m_signature->partial; }
    size_t raw_size_of_partial() const;

    // whole closure = nonlocals + partial
    size_t raw_size_of_closure() const { return raw_size_of_nonlocals() + raw_size_of_partial(); }
    size_t closure_size() const { return nonlocals().size() + partial().size(); }
    std::vector<TypeInfo> closure_types() const;

    // true if this function should be generic (i.e. signature contains a type variable)
    bool has_any_generic() const { return m_signature->has_any_generic(); }
    bool has_generic_params() const { return m_signature->has_generic_params(); }
    bool has_generic_return_type() const { return m_signature->has_generic_return_type(); }

    // Kind of function body

    // function has compiled bytecode and will be called or inlined
    struct CompiledBody {
        bool operator==(const CompiledBody& rhs) const;

        template<class Archive>
        void serialize(Archive& ar) {
            ar("code", code);
        }

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

        template<class Archive>
        void save(Archive& ar) const {
            ar("ast", ast());
        }

        template<class Archive>
        void load(Archive& ar) {
            ar(ast_copy);
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
    void set_code() { m_body = CompiledBody{}; m_compile = false; }

    void set_native(NativeDelegate native) { m_body = NativeBody{native}; }
    void call_native(Stack& stack) const { std::get<NativeBody>(m_body).native(stack); }

    bool is_undefined() const { return std::holds_alternative<std::monostate>(m_body); }
    bool has_code() const { return std::holds_alternative<CompiledBody>(m_body); }
    bool is_generic() const { return std::holds_alternative<GenericBody>(m_body); }
    bool is_native() const { return std::holds_alternative<NativeBody>(m_body); }

    void copy_body(const Function& src);

    void set_specialized() { m_specialized = true; }
    bool is_specialized() const { return m_specialized; }

    void set_compile(bool compile = true) { m_compile = compile; }
    bool has_compile() const { return m_compile; }

    void set_nonlocals_resolved(bool nonlocals_resolved = true) { m_nonlocals_resolved = nonlocals_resolved; }
    bool has_nonlocals_resolved() const { return m_nonlocals_resolved; }

    enum class Kind {
        Undefined,  // not yet compiled
        Compiled,   // compiled into bytecode
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
    std::shared_ptr<Signature> m_signature;
    // function body (depending on kind of function)
    std::variant<std::monostate, CompiledBody, GenericBody, NativeBody> m_body;
    // flags
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
        if (ti.is_unknown())
            return { m_type_args.end(), true };  // "inserted" Unknown to nowhere
        return m_type_args.try_emplace(sym, std::move(ti));
    }

    std::pair<iterator, bool> set(SymbolPointer sym, const TypeInfo& ti) {
        if (ti.is_unknown())
            return { m_type_args.end(), true };  // "inserted" Unknown to nowhere
        return m_type_args.try_emplace(sym, ti);
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
    Scope* parent() const { return m_parent_scope; }

    bool has_function() const { return m_function != no_index; }
    Function& function() const;
    void set_function_index(Index fn_idx) { m_function = fn_idx; }
    Index function_index() const { return m_function; }

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

    struct SpecArg {
        Index index;  // index from the respective Symbol::Parameter
        SourceLocation source_loc;
        SymbolPointer symptr;
    };

    void add_spec_arg(Index index, const SourceLocation& source_loc, SymbolPointer symptr);
    const SpecArg* get_spec_arg(Index index) const;
    const std::vector<SpecArg>& spec_args() const { return m_spec_args; }
    bool has_spec_args() const noexcept { return !m_spec_args.empty(); }

    const TypeArgs& type_args() const { return m_type_args; }
    TypeArgs& type_args() { return m_type_args; }
    bool has_type_args() const noexcept { return !m_type_args.empty(); }

private:
    Module* m_module = nullptr;
    Index m_function = no_index;  // function index in module
    Scope* m_parent_scope = nullptr;  // matches `m_symtab.parent()`, but can be a specialized function, while symtab is only lexical
    std::vector<Index> m_subscopes;  // nested scopes (Index into module scopes)
    std::vector<Nonlocal> m_nonlocals;
    std::vector<SpecArg> m_spec_args;  // if this scope is specialization of a function, this contains symbols passed as args (they can be generic functions)
    TypeArgs m_type_args;  // resolved type variables or explicit type args
};


} // namespace xci::script

#endif // include guard
