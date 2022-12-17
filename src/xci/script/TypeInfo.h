// TypeInfo.h created on 2019-06-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_TYPEINFO_H
#define XCI_SCRIPT_TYPEINFO_H

#include "SymbolTable.h"

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

    // Primitive types
    Bool,
    Byte,       // uint8
    Char,       // Unicode codepoint (char32)
    UInt32,
    UInt64,
    Int32,
    Int64,
    Float32,
    Float64,

    // Complex types
    String,     // special kind of list, behaves like [Char] but is compressed (UTF-8)
    List,       // list of same element type (elem type is part of type, size is part of value)
    Tuple,      // tuple of different value types
    //Variant,    // discriminated union (A|B|C)
    Function,   // function type, has signature (parameters, return type) and code
    Module,     // module type, carries global names, constants, functions
    Stream,     // I/O stream

    // Custom types
    Named,      // type NewType = ... (all other types are anonymous)
    Struct,     // a tuple with named members, similar to C struct
};


/// Decode Type from half-byte argument of an Opcode
/// See Opcode::Cast
Type decode_arg_type(uint8_t arg);

/// Return how many bytes a type occupies on the stack.
/// Returns 0 for Tuple -> cannot tell just from Type, need full TypeInfo for that.
size_t type_size_on_stack(Type type);


struct Signature;
struct NamedType;


class TypeInfo {
public:
    struct ListTag {};
    struct TupleTag {};
    struct StructTag {};
    static constexpr ListTag list_of {};
    static constexpr TupleTag tuple_of {};
    static constexpr StructTag struct_of {};

    using Var = SymbolPointer;  // for unknown type, specifies which type variable this represents
    using Subtypes = std::vector<TypeInfo>;
    using StructItem = std::pair<std::string, TypeInfo>;
    using StructItems = std::vector<StructItem>;
    using SignaturePtr = std::shared_ptr<Signature>;
    using NamedTypePtr = std::shared_ptr<NamedType>;

    // Unknown / generic
    TypeInfo() = default;
    explicit TypeInfo(Var var) : m_info(var) {}
    // Plain types
    explicit TypeInfo(Type type);
    // Function
    explicit TypeInfo(SignaturePtr signature)
        : m_type(Type::Function), m_info(std::move(signature)) {}
    // List
    explicit TypeInfo(ListTag, TypeInfo list_elem);
    // Tuple
    explicit TypeInfo(TupleTag, std::initializer_list<TypeInfo> subtypes)
        : m_type(Type::Tuple), m_info(Subtypes(subtypes)) {}
    explicit TypeInfo(Subtypes subtypes)
        : m_type(Type::Tuple), m_info(std::move(subtypes)) {}
    // Struct
    explicit TypeInfo(StructTag, std::initializer_list<StructItem> items)
            : m_type(Type::Struct), m_info(StructItems(items)) {}
    explicit TypeInfo(StructItems items)
            : m_type(Type::Struct), m_info(std::move(items)) {}
    // Named
    explicit TypeInfo(std::string name, TypeInfo&& type_info);

    TypeInfo(const TypeInfo&) = default;
    TypeInfo& operator =(const TypeInfo&) = default;
    TypeInfo(TypeInfo&& other) noexcept;
    TypeInfo& operator =(TypeInfo&& other) noexcept;

    size_t size() const;
    void foreach_heap_slot(std::function<void(size_t offset)> cb) const;

    Type type() const { return m_type; }
    Type underlying_type() const;
    bool is_unknown() const { return m_type == Type::Unknown; }
    bool is_named() const { return m_type == Type::Named; }
    bool is_callable() const { return underlying_type() == Type::Function; }
    bool is_void() const { return is_tuple() && subtypes().empty(); }
    bool is_bool() const { return underlying_type() == Type::Bool; }
    bool is_tuple() const { return underlying_type() == Type::Tuple; }
    bool is_struct() const { return underlying_type() == Type::Struct; }

    bool is_generic() const;
    void replace_var(SymbolPointer var, const TypeInfo& ti);

    // If the type is function without args, get its return type.
    const TypeInfo& effective_type() const;

    /// Deep comparison of underlying types. The types must be fully known, non-generic.
    /// \returns true if the types are equivalent (same size, layout, underlying types)
    friend bool is_same_underlying(const TypeInfo& lhs, const TypeInfo& rhs);

    bool operator==(const TypeInfo& rhs) const;
    bool operator!=(const TypeInfo& rhs) const { return !(*this == rhs); }

    explicit operator bool() const { return m_type != Type::Unknown || generic_var(); }

    // -------------------------------------------------------------------------
    // Additional info, subtypes

    Var generic_var() const;  // type = Unknown
    const TypeInfo& elem_type() const;  // type = List (Subtypes[0])
    TypeInfo& elem_type();              // type = List (Subtypes[0])
    const Subtypes& subtypes() const;  // type = Tuple
    Subtypes& subtypes();              // type = Tuple
    const StructItems& struct_items() const;  // type = Struct
    const TypeInfo* struct_item_by_name(const std::string& name) const;  // type = Struct
    Subtypes struct_or_tuple_subtypes() const;  // type = Tuple | Struct
    const SignaturePtr& signature_ptr() const;  // type = Function
    const Signature& signature() const { return *signature_ptr(); }
    Signature& signature() { return *signature_ptr(); }
    const NamedTypePtr& named_type_ptr() const;   // type = Named
    const NamedType& named_type() const { return *named_type_ptr(); }
    const TypeInfo& underlying() const;  // transparently get type_info of NamedType
    std::string name() const;

    // -------------------------------------------------------------------------
    // Serialization

    template <class Archive>
    void save_schema(Archive& ar) const {
        ar("type", m_type);
        if (ar.enter_union("info", "type", typeid(decltype(m_info)))) {
            ar(uint8_t(Type::Unknown), "var", SymbolPointer{});
            ar(uint8_t(Type::List), "elem_type", TypeInfo{});
            ar(uint8_t(Type::Tuple), "subtypes", Subtypes{});
            ar(uint8_t(Type::Function), "signature", SignaturePtr{});
            ar(uint8_t(Type::Named), "named_type", NamedTypePtr{});
            ar(uint8_t(Type::Struct), "struct_items", StructItems{});
            ar.leave_union();
        }
    }

    template <class Archive>
    void save(Archive& ar) const {
        ar("type", m_type);
        switch (m_type) {
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
                ar("subtypes", subtypes());
                break;
            case Type::Struct:
                ar("struct_items", struct_items());
                break;
            case Type::Named:
                ar("named_type", named_type());
                break;
            default:
                break;
        }
    }

    template <class Archive>
    void load(Archive& ar)
    {
        ar(m_type);
        switch (m_type) {
            case Type::Unknown: {
                m_info.emplace<Var>();
                ar(std::get<Var>(m_info));
                break;
            }
            case Type::Function: {
                auto signature = std::make_shared<Signature>();
                ar(*signature);
                m_info = std::move(signature);
                break;
            }
            case Type::List: {
                m_info.emplace<Subtypes>();
                auto& subtypes = std::get<Subtypes>(m_info);
                subtypes.emplace_back();
                ar(subtypes.back());
                break;
            }
            case Type::Tuple: {
                m_info.emplace<Subtypes>();
                ar(std::get<Subtypes>(m_info));
                break;
            }
            case Type::Struct: {
                m_info.emplace<StructItems>();
                ar(std::get<StructItems>(m_info));
                break;
            }
            case Type::Named: {
                auto named_type = std::make_shared<NamedType>();
                ar(named_type);
                m_info.emplace<NamedTypePtr>(std::move(named_type));
                break;
            }
            default:
                break;
        }
    }

private:
    Type m_type { Type::Unknown };
    std::variant<Var, Subtypes, StructItems, SignaturePtr, NamedTypePtr> m_info;
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

    bool has_generic_params() const;
    bool has_generic_return_type() const;
    bool has_generic_nonlocals() const;
    bool has_nonvoid_params() const;
    bool has_any_generic() const { return has_generic_params() || has_generic_return_type() || has_generic_nonlocals(); }

    unsigned arity() const noexcept { return params.size(); }

    explicit operator bool() const { return !params.empty() || return_type; }

    bool operator==(const Signature& rhs) const = default;
    bool operator!=(const Signature& rhs) const = default;

    template <class Archive>
    void serialize(Archive& ar) {
        ar ("nonlocals", nonlocals) ("partial", partial)
           ("params", params) ("return_type", return_type);
    }
};


struct NamedType {
    std::string name;
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
inline TypeInfo ti_uint32() { return TypeInfo(Type::UInt32); }
inline TypeInfo ti_uint64() { return TypeInfo(Type::UInt64); }
inline TypeInfo ti_int32() { return TypeInfo(Type::Int32); }
inline TypeInfo ti_int64() { return TypeInfo(Type::Int64); }
inline TypeInfo ti_float32() { return TypeInfo(Type::Float32); }
inline TypeInfo ti_float64() { return TypeInfo(Type::Float64); }
inline TypeInfo ti_string() { return TypeInfo(Type::String); }
inline TypeInfo ti_stream() { return TypeInfo(Type::Stream); }
inline TypeInfo ti_module() { return TypeInfo(Type::Module); }

inline TypeInfo ti_function(std::shared_ptr<Signature>&& signature)
{ return TypeInfo(std::forward<std::shared_ptr<Signature>>(signature)); }

inline TypeInfo ti_list(TypeInfo&& elem) { return TypeInfo(TypeInfo::list_of, std::forward<TypeInfo>(elem)); }
inline TypeInfo ti_chars() { return TypeInfo{TypeInfo::list_of, ti_char()}; }
inline TypeInfo ti_bytes() { return TypeInfo{TypeInfo::list_of, ti_byte()}; }

// Each item must be TypeInfo
template <typename... Args>
inline TypeInfo ti_tuple(Args&&... args) { return TypeInfo(TypeInfo::tuple_of, {std::forward<TypeInfo>(args)...}); }

// Each item must be std::pair<std::string, TypeInfo>
inline TypeInfo ti_struct(std::initializer_list<TypeInfo::StructItem> items)
{ return TypeInfo(TypeInfo::struct_of, std::forward<std::initializer_list<TypeInfo::StructItem>>(items)); }


} // namespace xci::script

#endif // include guard
