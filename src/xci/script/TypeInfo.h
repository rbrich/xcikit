// TypeInfo.h created on 2019-06-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_TYPEINFO_H
#define XCI_SCRIPT_TYPEINFO_H

#include <cstdint>
#include <ostream>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <utility>
#include <variant>

namespace xci::script {


enum class Type : uint8_t {
    Unknown,    // type not known at this time (might be inferred or generic)
    Void,       // void type - has no value

    // Plain types
    Bool,
    Byte,       // uint8
    Char,       // Unicode codepoint (char32)
    Int32,
    Int64,
    Float32,
    Float64,

    // Complex types
    String,     // special kind of list, behaves like [Char] but is compressed (UTF-8)
    List,       // list of same element type (elem type is part of type, size is part of value)
    Tuple,      // tuple of different value types
    Function,   // function type, has signature (parameters, return type) and code
    Module,     // module type, carries global names, constants, functions
    //Data,       // user defined data type
};


/// Decode Type from half-byte argument of an Opcode
/// See Opcode::Cast
Type decode_arg_type(uint8_t arg);

/// Return how many bytes a type occupies on the stack.
/// Returns 0 for Tuple -> cannot tell just from Type, need full TypeInfo for that.
size_t type_size_on_stack(Type type);


struct Signature;

class TypeInfo {
public:
    // Unknown / generic
    TypeInfo() : m_type(Type::Unknown), m_info(Var(0)) {}
    explicit TypeInfo(uint8_t var) : m_type(Type::Unknown), m_info(var) {}
    // Plain types
    explicit TypeInfo(Type type);
    // Function
    explicit TypeInfo(std::shared_ptr<Signature> signature)
        : m_type(Type::Function), m_info(std::move(signature)) {}
    // Tuple
    explicit TypeInfo(std::vector<TypeInfo> tuple_subtypes)
        : m_type(Type::Tuple), m_info(std::move(tuple_subtypes)) {}
    // List
    explicit TypeInfo(Type t, TypeInfo list_elem);

    // shortcuts
    static TypeInfo bytes() { return TypeInfo{Type::List,TypeInfo{Type::Byte}}; }

    size_t size() const;
    void foreach_heap_slot(std::function<void(size_t offset)> cb) const;

    Type type() const { return m_type; }
    bool is_callable() const { return m_type == Type::Function; }
    bool is_unknown() const { return m_type == Type::Unknown; }
    bool is_void() const { return m_type == Type::Void; }

    void replace_var(uint8_t idx, const TypeInfo& ti);

    TypeInfo effective_type() const;

    bool operator==(const TypeInfo& rhs) const;
    bool operator!=(const TypeInfo& rhs) const { return !(*this == rhs); }

    explicit operator bool() const { return m_type != Type::Unknown; }

    // -------------------------------------------------------------------------
    // Additional info, subtypes

    using Var = uint8_t;  // for unknown type, specifies which type variable this represents (counted from 1, none = 0)
    using Subtypes = std::vector<TypeInfo>;
    using SignaturePtr = std::shared_ptr<Signature>;

    Var generic_var() const;  // type = Unknown
    const TypeInfo& elem_type() const;  // type = List (Subtypes[0])
    const Subtypes& subtypes() const;  // type = Tuple
    const SignaturePtr& signature_ptr() const;  // type = Function
    const Signature& signature() const { return *signature_ptr(); }
    Signature& signature() { return *signature_ptr(); }

private:
    Type m_type { Type::Unknown };
    std::variant<std::monostate, Var, Subtypes, SignaturePtr> m_info;
};


struct Signature {
    std::vector<TypeInfo> nonlocals;
    std::vector<TypeInfo> partial;
    std::vector<TypeInfo> params;
    TypeInfo return_type;

    void add_nonlocal(TypeInfo&& ti) { nonlocals.emplace_back(ti); }
    void add_partial(TypeInfo&& ti) { partial.emplace_back(ti); }
    void add_parameter(TypeInfo&& ti) { params.emplace_back(ti); }
    void set_return_type(TypeInfo ti) { return_type = std::move(ti); }

    bool has_closure() const { return !nonlocals.empty() || !partial.empty(); }

    // Check return type matches and set it to concrete type if it's generic.
    void resolve_return_type(const TypeInfo& t);

    bool operator==(const Signature& rhs) const {
        return params == rhs.params
            && partial == rhs.partial
            && nonlocals == rhs.nonlocals
            && return_type == rhs.return_type;
    }
    bool operator!=(const Signature& rhs) const { return !(rhs == *this); }
};


} // namespace xci::script

#endif // include guard
