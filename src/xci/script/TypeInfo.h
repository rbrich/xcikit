// TypeInfo.h created on 2019-06-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2026 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_TYPEINFO_H
#define XCI_SCRIPT_TYPEINFO_H

#include "ValueId.h"
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


class TypeInfo {
public:
    struct UnknownTag {};
    struct NamedTag {};
    struct ListTag {};
    struct TupleTag {};
    struct StructTag {};

    static constexpr UnknownTag unknown {};
    static constexpr NamedTag named {};
    static constexpr ListTag list_of {};
    static constexpr TupleTag tuple_of {};
    static constexpr StructTag struct_of {};

    using Subtypes = core::StaticVec<TypeInfo>;
    using SignaturePtr = std::shared_ptr<Signature>;

    // Unknown / generic
    TypeInfo() { construct_variant(); }
    ~TypeInfo() { destroy_variant(); }

    // Unknown type, specifies which type variable this represents
    explicit TypeInfo(UnknownTag, SymbolPointer var) : m_type(Type::Unknown) { std::construct_at(&m_symptr, var); }
    // Default-construct any type
    explicit TypeInfo(Type type) : m_type(type) { construct_variant(); }
    // Function
    explicit TypeInfo(SignaturePtr signature) : m_type(Type::Function) { std::construct_at(&m_signature_ptr, std::move(signature)); }
    // List
    explicit TypeInfo(ListTag tag, TypeInfo elem);
    // Tuple
    explicit TypeInfo(TupleTag, Subtypes subtypes)
            : m_type(Type::Tuple) { std::construct_at(&m_subtypes, std::move(subtypes)); }
    explicit TypeInfo(Subtypes subtypes)
            : m_type(Type::Tuple) { std::construct_at(&m_subtypes, std::move(subtypes)); }
    // Struct
    explicit TypeInfo(StructTag, Subtypes subtypes)
            : m_type(Type::Struct) { std::construct_at(&m_subtypes, std::move(subtypes)); }
    // Named
    explicit TypeInfo(NamedTag, SymbolPointer symptr) : m_type(Type::Named) { std::construct_at(&m_symptr, symptr); }

    TypeInfo(const TypeInfo& r)
            : m_type(r.m_type), m_is_literal(r.m_is_literal), m_key(r.m_key), m_default_value(r.m_default_value)
    {
        assert(this != &r);
        construct_variant();
        copy_variant(r);
    }
    TypeInfo(TypeInfo&& r) noexcept
            : m_type(r.m_type), m_is_literal(r.m_is_literal), m_refcount(r.m_refcount),
              m_key(r.m_key), m_default_value(r.m_default_value)
    {
        assert(this != &r);
        construct_variant();
        move_variant(std::move(r));
        r.set_type(Type::Unknown);
        r.m_symptr = {};
        r.m_is_literal = true;
        r.m_refcount = 0;
        r.m_key = {};
        r.m_default_value = ValueId{};
    }

    TypeInfo& operator =(const TypeInfo& r);
    TypeInfo& operator =(TypeInfo&& r) noexcept;

    // Assign type info, but keep key untouched
    void copy_from_no_key(const TypeInfo& r) noexcept;

    size_t size() const;
    void foreach_heap_slot(std::function<void(size_t offset)> cb) const;

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

    bool is_unknown() const { return type() == Type::Unknown; }
    bool is_unspecified() const { return is_unknown() && !symptr(); }  // == is_unknown() && !is_generic()
    bool is_generic() const { return is_unknown() && bool(symptr()); }
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

    explicit operator bool() const { return type() != Type::Unknown || symptr(); }

    // -------------------------------------------------------------------------
    // Additional info, subtypes

    NameId key() const { return m_key; }  // name of item inside Struct
    void set_key(NameId key) { m_key = key; }
    const TypeInfo* struct_item_by_key(NameId key) const;  // type = Struct

    ValueId default_value() const { return m_default_value; }  // default value of struct item or function parameter
    void set_default_value(ValueId val) { m_default_value = val; }

    SymbolPointer symptr() const;  // type = Unknown
    SymbolPointer& symptr();  // type = Unknown
    const TypeInfo& elem_type() const;  // type = List (Subtypes[0])
    TypeInfo& elem_type() { return const_cast<TypeInfo&>( const_cast<const TypeInfo*>(this)->elem_type() ); }
    const Subtypes& subtypes() const;  // type = Tuple
    Subtypes& subtypes() { return const_cast<Subtypes&>( const_cast<const TypeInfo*>(this)->subtypes() ); }
    const SignaturePtr& signature_ptr() const;  // type = Function
    SignaturePtr& signature_ptr() { return const_cast<SignaturePtr&>( const_cast<const TypeInfo*>(this)->signature_ptr() ); }
    const Signature& signature() const { return *signature_ptr(); }
    Signature& signature() { return *signature_ptr(); }
    NameId name() const;   // type = Named
    const TypeInfo& named_type() const;
    const TypeInfo& underlying() const;  // transparently get type_info of NamedType
    const SignaturePtr& ul_signature_ptr() const { return underlying().signature_ptr(); }
    const Signature& ul_signature() const { return underlying().signature(); }

    void set_refcount(uint16_t refcount) { m_refcount = refcount; }
    uint16_t refcount() const { return m_refcount; }
    uint16_t incref() { return ++m_refcount; }
    uint16_t decref() { return --m_refcount; }

private:
    void construct_variant();
    void destroy_variant();
    void copy_variant(const TypeInfo& r);
    void move_variant(TypeInfo&& r);

    /// Discriminated union.
    /// In comparison to std::variant, this saves 8 bytes + some unnecessary templating.
    union {
        SymbolPointer m_symptr;
        Subtypes m_subtypes;
        SignaturePtr m_signature_ptr;
    };
    Type m_type { Type::Unknown };

    bool m_is_literal = true;  // literal = any expression that doesn't reference functions/variables
    uint16_t m_refcount = 0;
    NameId m_key;
    ValueId m_default_value;
};


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

    template <class Archive>
    void serialize(Archive& ar) {
        ar ("nonlocals", nonlocals)
           ("param_type", param_type)
           ("return_type", return_type);
    }
};

using SignaturePtr = std::shared_ptr<Signature>;


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
inline TypeInfo ti_key(std::string_view key, TypeInfo&& ti, ValueId default_value = ValueId()) {
    ti.set_key(intern(key));
    ti.set_default_value(default_value);
    return std::move(ti);
}

// Normalize: unwrap tuple of one item
inline TypeInfo ti_normalize(TypeInfo&& ti)
{ return ti.is_tuple() && ti.subtypes().size() == 1 ? ti.subtypes().front() : ti; }


} // namespace xci::script

#endif // include guard
