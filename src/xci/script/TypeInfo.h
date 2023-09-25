// TypeInfo.h created on 2019-06-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_TYPEINFO_H
#define XCI_SCRIPT_TYPEINFO_H

#include "SymbolTable.h"
#include <xci/core/container/StaticVec.h>

#include <cstdint>
#include <ostream>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <utility>

namespace xci::script {


enum class Type : uint8_t {
    Unknown,    // type not known at this time (might be inferred or generic)

    // Primitive types
    Bool,
    Char,       // Unicode codepoint (char32)
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    UInt128,
    Int8,
    Int16,
    Int32,
    Int64,
    Int128,
    Float32,
    Float64,
    Float128,

    // Complex types
    String,     // special kind of list, behaves like [Char] but is compressed (UTF-8)
    List,       // list of same element type (elem type is part of type, size is part of value)
    Tuple,      // tuple of different value types
    //Variant,    // discriminated union (A|B|C)
    Function,   // function type, has signature (parameters, return type) and code
    Module,     // module type, carries global names, constants, functions
    Stream,     // I/O stream
    TypeIndex,   // index of a type in current module or builtin module

    // Custom types
    Named,      // type NewType = ... (all other types are anonymous)
    Struct,     // a tuple with named members, similar to C struct

    // Aliases:
    Byte = UInt8,
    Int = Int64,
    UInt = UInt64,
    Float = Float64,
};


/// Decode Type from half-byte argument of an Opcode
/// See Opcode::Cast
Type decode_arg_type(uint8_t arg);

/// Return how many bytes a type occupies on the stack.
/// Returns 0 for Tuple -> cannot tell just from Type, need full TypeInfo for that.
size_t type_size_on_stack(Type type);


struct Signature;
struct NamedType;


namespace detail {

/// Discriminated union for TypeInfo details.
/// In comparison to std::variant, this saves 8 bytes + some unnecessary templating.
class TypeInfoUnion {
public:
    struct ListTag {};
    struct TupleTag {};
    struct StructTag {};

    using Var = SymbolPointer;  // for unknown type, specifies which type variable this represents
    using Subtypes = core::StaticVec<TypeInfo>;
    using SignaturePtr = std::shared_ptr<Signature>;
    using NamedTypePtr = std::shared_ptr<NamedType>;

    Type type() const noexcept { return m_type; }
    void set_type(Type type) {
        if (m_type == type)
            return;
        if ((m_type == Type::List || m_type == Type::Tuple || m_type == Type::Struct) &&
            (type == Type::List || type == Type::Tuple || type == Type::Struct))
        {
            m_type = type;
            return;
        }
        destroy_variant();
        m_type = type;
        construct_variant();
    }

    Var generic_var() const;  // type = Unknown
    Var& generic_var();  // type = Unknown
    const Subtypes& subtypes() const;  // type = Tuple
    Subtypes& subtypes() { return const_cast<Subtypes&>( const_cast<const TypeInfoUnion*>(this)->subtypes() ); }
    const SignaturePtr& signature_ptr() const;  // type = Function
    SignaturePtr& signature_ptr() { return const_cast<SignaturePtr&>( const_cast<const TypeInfoUnion*>(this)->signature_ptr() ); }
    const NamedTypePtr& named_type_ptr() const;   // type = Named

protected:
    TypeInfoUnion() { construct_variant(); }
    TypeInfoUnion(Var var) { std::construct_at(&m_var, var); }
    TypeInfoUnion(Type type) : m_type(type) { construct_variant(); }
    TypeInfoUnion(SignaturePtr signature) : m_type(Type::Function) { std::construct_at(&m_signature_ptr, std::move(signature)); }
    TypeInfoUnion(ListTag, TypeInfo elem);
    TypeInfoUnion(TupleTag, Subtypes subtypes) : m_type(Type::Tuple) { std::construct_at(&m_subtypes, std::move(subtypes)); }
    TypeInfoUnion(StructTag, Subtypes subtypes) : m_type(Type::Struct) { std::construct_at(&m_subtypes, std::move(subtypes)); }
    TypeInfoUnion(NameId name, TypeInfo&& type_info);
    ~TypeInfoUnion() { destroy_variant(); }

    TypeInfoUnion(const TypeInfoUnion& r) : m_type(r.m_type) { construct_variant(); *this = r; }
    TypeInfoUnion& operator =(const TypeInfoUnion& r);

    TypeInfoUnion(TypeInfoUnion&& r) noexcept : m_type(r.m_type) { construct_variant(); *this = std::move(r); }
    TypeInfoUnion& operator =(TypeInfoUnion&& r) noexcept;

private:
    void construct_variant();
    void destroy_variant();

    union {
        Var m_var;
        Subtypes m_subtypes;
        SignaturePtr m_signature_ptr;
        NamedTypePtr m_named_type_ptr;
    };
    Type m_type { Type::Unknown };
};

}  // namespace detail


class TypeInfo: public detail::TypeInfoUnion {
public:
    static constexpr ListTag list_of {};
    static constexpr TupleTag tuple_of {};
    static constexpr StructTag struct_of {};

    // Unknown / generic
    TypeInfo() = default;
    explicit TypeInfo(Var var) : detail::TypeInfoUnion(var) {}
    // Default-construct any type
    explicit TypeInfo(Type type) : detail::TypeInfoUnion(type) {}
    // Function
    explicit TypeInfo(SignaturePtr signature) : detail::TypeInfoUnion(std::move(signature)) {}
    // List
    explicit TypeInfo(ListTag tag, TypeInfo elem) : detail::TypeInfoUnion(tag, std::move(elem)) {}
    // Tuple
    explicit TypeInfo(TupleTag tag, Subtypes subtypes)
            : detail::TypeInfoUnion(tuple_of, std::move(subtypes)) {}
    explicit TypeInfo(Subtypes subtypes)
            : detail::TypeInfoUnion(tuple_of, std::move(subtypes)) {}
    // Struct
    explicit TypeInfo(StructTag, Subtypes subtypes)
            : detail::TypeInfoUnion(struct_of, std::move(subtypes)) {}
    // Named
    explicit TypeInfo(NameId name, TypeInfo&& type_info)
            : detail::TypeInfoUnion(name, std::move(type_info)) {}

    TypeInfo(const TypeInfo&) = default;
    TypeInfo(TypeInfo&& r) noexcept
            : detail::TypeInfoUnion(std::move(r)), m_is_literal(r.m_is_literal), m_key(r.m_key)
    {
        assert(this != &r);
        r.m_is_literal = true;
        r.m_key = {};
    }

    TypeInfo& operator =(const TypeInfo& r) = default;
    TypeInfo& operator =(TypeInfo&& r) noexcept {
        assert(this != &r);
        this->detail::TypeInfoUnion::operator=(std::move(r));
        m_is_literal = r.m_is_literal; r.m_is_literal = true;
        m_key = r.m_key; r.m_key = {};
        return *this;
    }

    // Do not clear key (but copy it from `r` if it has a key)
    void assign_from(const TypeInfo& r) {
        this->detail::TypeInfoUnion::operator=(r);
        m_is_literal = r.m_is_literal;
        if (r.m_key)
            m_key = r.m_key;
    }

    size_t size() const;
    void foreach_heap_slot(std::function<void(size_t offset)> cb) const;

    bool is_unknown() const { return type() == Type::Unknown; }
    bool is_unspecified() const { return is_unknown() && !generic_var(); }  // == is_unknown() && !is_generic()
    bool is_generic() const { return is_unknown() && bool(generic_var()); }
    bool is_named() const { return type() == Type::Named; }
    bool is_function() const { return type() == Type::Function; }
    bool is_callable() const { return underlying().is_function(); }
    bool is_void() const { return is_tuple() && subtypes().empty(); }
    bool is_bool() const { return type() == Type::Bool; }
    bool is_string() const { return type() == Type::String; }
    bool is_list() const { return type() == Type::List; }
    bool is_tuple() const { return type() == Type::Tuple; }
    bool is_struct() const { return type() == Type::Struct; }
    bool is_struct_or_tuple() const { return is_struct() || is_tuple(); }

    bool has_unknown() const;  // deep check, e.g. ?, [?], Int->?
    bool has_generic() const;  // deep check, e.g. T, [T], Int->T
    void replace_var(SymbolPointer var, const TypeInfo& ti);

    bool is_literal() const { return m_is_literal; }
    void set_literal(bool literal) { m_is_literal = literal; }

    // If the type is function without args, get its return type.
    const TypeInfo& effective_type() const;

    /// Deep comparison of underlying types. The types must be fully known, non-generic.
    /// \returns true if the types are equivalent (same size, layout, underlying types)
    friend bool is_same_underlying(const TypeInfo& lhs, const TypeInfo& rhs);

    bool operator==(const TypeInfo& rhs) const;
    bool operator!=(const TypeInfo& rhs) const { return !(*this == rhs); }

    explicit operator bool() const { return type() != Type::Unknown || generic_var(); }

    // -------------------------------------------------------------------------
    // Additional info, subtypes

    NameId key() const { return m_key; }  // name of item inside Struct
    void set_key(NameId key) { m_key = key; }
    const TypeInfo* struct_item_by_key(NameId key) const;  // type = Struct

    const TypeInfo& elem_type() const;  // type = List (Subtypes[0])
    TypeInfo& elem_type() { return const_cast<TypeInfo&>( const_cast<const TypeInfo*>(this)->elem_type() ); }
    const Signature& signature() const { return *signature_ptr(); }
    Signature& signature() { return *signature_ptr(); }
    const NamedType& named_type() const { return *named_type_ptr(); }
    NamedType& named_type() { return *named_type_ptr(); }
    NameId name() const;
    const TypeInfo& underlying() const;  // transparently get type_info of NamedType
    TypeInfo& underlying();
    const SignaturePtr& ul_signature_ptr() const { return underlying().signature_ptr(); }
    const Signature& ul_signature() const { return underlying().signature(); }

    // -------------------------------------------------------------------------
    // Serialization

    template <class Archive> void save_schema(Archive& ar) const;
    template <class Archive> void save(Archive& ar) const;
    template <class Archive> void load(Archive& ar);

private:
    bool m_is_literal = true;  // literal = any expression that doesn't reference functions/variables
    NameId m_key;
};


template <class Archive>
void TypeInfo::save_schema(Archive& ar) const {
    ar("key", m_key);
    ar("type", type());
    if (ar.enter_union("info", "type", typeid(TypeInfoUnion))) {
        ar(uint8_t(Type::Unknown), "var", SymbolPointer{});
        ar(uint8_t(Type::List), "elem_type", TypeInfo{});
        ar(uint8_t(Type::Tuple), "subtypes", Subtypes{});
        ar(uint8_t(Type::Function), "signature", SignaturePtr{});
        ar(uint8_t(Type::Named), "named_type", NamedTypePtr{});
        ar(uint8_t(Type::Struct), "subtypes", Subtypes{});
        ar.leave_union();
    }
}


template <class Archive>
void TypeInfo::save(Archive& ar) const {
    if (m_key)
        ar("key", m_key);
    ar("type", type());
    switch (type()) {
        case Type::Unknown:
            ar("var", generic_var());
            break;
        case Type::Function:
            ar("signature", signature());
            break;
        case Type::List:
            ar("elem_type", elem_type());
            break;
        case Type::Tuple:
        case Type::Struct:
            ar("subtypes", subtypes());
            break;
        case Type::Named:
            ar("named_type", named_type());
            break;
        default:
            break;
    }
}


template <class Archive>
void TypeInfo::load(Archive& ar)
{
    ar(m_key);
    Type t;
    ar(t);
    set_type(t);
    switch (type()) {
        case Type::Unknown: {
            ar(generic_var());
            break;
        }
        case Type::Function: {
            ar(*signature_ptr());
            break;
        }
        case Type::List: {
            subtypes().resize(1);
            ar(subtypes().front());
            break;
        }
        case Type::Tuple:
        case Type::Struct: {
            std::vector<TypeInfo> v;
            ar(v);
            subtypes() = Subtypes(core::to_span(v));
            break;
        }
        case Type::Named: {
            ar(named_type());
            break;
        }
        default:
            break;
    }
}


struct Signature {
    std::vector<TypeInfo> nonlocals;
    TypeInfo param_type;
    TypeInfo return_type;

    void add_nonlocal(TypeInfo&& ti) { nonlocals.emplace_back(ti); }
    void set_parameter(TypeInfo ti) { param_type = std::move(ti); }
    void set_return_type(TypeInfo ti) { return_type = std::move(ti); return_type.set_literal(false); }

    bool has_closure() const { return !nonlocals.empty(); }

    bool has_generic_nonlocals() const;
    bool has_unknown_nonlocals() const;
    bool has_nonvoid_param() const;
    bool has_any_unknown() const { return param_type.has_unknown() || return_type.has_unknown() || has_unknown_nonlocals(); }
    bool has_any_generic() const { return param_type.has_generic() || return_type.has_generic() || has_generic_nonlocals(); }

    explicit operator bool() const { return param_type || return_type; }

    bool operator==(const Signature& rhs) const = default;
    bool operator!=(const Signature& rhs) const = default;

    template <class Archive>
    void serialize(Archive& ar) {
        ar ("nonlocals", nonlocals)
           ("param_type", param_type)
           ("return_type", return_type);
    }
};

using SignaturePtr = std::shared_ptr<Signature>;


struct NamedType {
    NameId name;
    TypeInfo type_info;

    bool operator==(const NamedType& rhs) const = default;
    bool operator!=(const NamedType& rhs) const = default;

    template <class Archive>
    void serialize(Archive& ar) {
        ar ("name", name) ("type_info", type_info);
    }
};


// -----------------------------------------------------------------------------
// Shortcuts

inline TypeInfo ti_unknown() { return TypeInfo(Type::Unknown); }
inline TypeInfo ti_void() { return TypeInfo(TypeInfo::tuple_of, {}); }
inline TypeInfo ti_bool() { return TypeInfo(Type::Bool); }
inline TypeInfo ti_byte() { return TypeInfo(Type::Byte); }
inline TypeInfo ti_char() { return TypeInfo(Type::Char); }
inline TypeInfo ti_uint() { return TypeInfo(Type::UInt); }
inline TypeInfo ti_uint8() { return TypeInfo(Type::UInt8); }
inline TypeInfo ti_uint16() { return TypeInfo(Type::UInt16); }
inline TypeInfo ti_uint32() { return TypeInfo(Type::UInt32); }
inline TypeInfo ti_uint64() { return TypeInfo(Type::UInt64); }
inline TypeInfo ti_uint128() { return TypeInfo(Type::UInt128); }
inline TypeInfo ti_int() { return TypeInfo(Type::Int); }
inline TypeInfo ti_int8() { return TypeInfo(Type::Int8); }
inline TypeInfo ti_int16() { return TypeInfo(Type::Int16); }
inline TypeInfo ti_int32() { return TypeInfo(Type::Int32); }
inline TypeInfo ti_int64() { return TypeInfo(Type::Int64); }
inline TypeInfo ti_int128() { return TypeInfo(Type::Int128); }
inline TypeInfo ti_float() { return TypeInfo(Type::Float); }
inline TypeInfo ti_float32() { return TypeInfo(Type::Float32); }
inline TypeInfo ti_float64() { return TypeInfo(Type::Float64); }
inline TypeInfo ti_float128() { return TypeInfo(Type::Float128); }
inline TypeInfo ti_string() { return TypeInfo(Type::String); }
inline TypeInfo ti_stream() { return TypeInfo(Type::Stream); }
inline TypeInfo ti_module() { return TypeInfo(Type::Module); }
inline TypeInfo ti_type_index() { return TypeInfo(Type::TypeIndex); }

inline TypeInfo ti_function(SignaturePtr&& signature)
{ return TypeInfo(std::forward<SignaturePtr>(signature)); }

inline TypeInfo ti_list(TypeInfo&& elem) { return TypeInfo(TypeInfo::list_of, std::forward<TypeInfo>(elem)); }
inline TypeInfo ti_chars() { return TypeInfo{TypeInfo::list_of, ti_char()}; }
inline TypeInfo ti_bytes() { return TypeInfo{TypeInfo::list_of, ti_byte()}; }

// Each item must be TypeInfo
template <typename... Args>
inline TypeInfo ti_tuple(Args&&... args) { return TypeInfo(TypeInfo::tuple_of, {std::forward<TypeInfo>(args)...}); }

// Each item should have key
inline TypeInfo ti_struct(std::initializer_list<TypeInfo> items)
{ return TypeInfo(TypeInfo::struct_of, std::forward<std::initializer_list<TypeInfo>>(items)); }
inline TypeInfo ti_keyed(NameId key, TypeInfo&& ti) {
    ti.set_key(key);
    return std::move(ti);
}

// Normalize: unwrap tuple of one item
inline TypeInfo ti_normalize(TypeInfo&& ti)
{ return ti.is_tuple() && ti.subtypes().size() == 1 ? ti.subtypes().front() : ti; }


} // namespace xci::script

#endif // include guard
