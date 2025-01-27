// TypeInfo.cpp created on 2019-06-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "TypeInfo.h"
#include "Error.h"
#include <xci/compat/macros.h>
#include <ranges>
#include <numeric>

namespace xci::script {


Type decode_arg_type(uint8_t arg)
{
    switch (arg) {
        case 0x1: return Type::UInt8;
        case 0x2: return Type::UInt16;
        case 0x3: return Type::UInt32;
        case 0x4: return Type::UInt64;
        case 0x5: return Type::UInt128;
        case 0x6: return Type::Int8;
        case 0x7: return Type::Int16;
        case 0x8: return Type::Int32;
        case 0x9: return Type::Int64;
        case 0xA: return Type::Int128;
        // 0xB is reserved for Float16
        case 0xC: return Type::Float32;
        case 0xD: return Type::Float64;
        case 0xE: return Type::Float128;
        default: return Type::Unknown;
    }
}


size_t type_size_on_stack(Type type)
{
    switch (type) {
        case Type::Unknown:
        case Type::Tuple:
        case Type::Struct:
        case Type::Named:
            return 0;
        case Type::Bool:
        case Type::UInt8:
        case Type::Int8:
            return 1;
        case Type::UInt16:
        case Type::Int16:
            return 2;
        case Type::Char:
        case Type::UInt32:
        case Type::Int32:
        case Type::Float32:
        case Type::TypeIndex:
            return 4;
        case Type::UInt64:
        case Type::Int64:
        case Type::Float64:
            return 8;
        case Type::UInt128:
        case Type::Int128:
        case Type::Float128:
            return 16;
        case Type::String:
        case Type::List:
        case Type::Function:
        case Type::Stream:
        case Type::Module:
            return sizeof(void*);
    }
    return 0;
}


// -------------------------------------------------------------------------------------------------


TypeInfo::TypeInfo(ListTag, TypeInfo elem)
        : m_type(Type::List)
{
    std::construct_at(&m_subtypes, Subtypes({std::move(elem)}));
}


TypeInfo::TypeInfo(NameId name, TypeInfo&& type_info)
        : m_type(Type::Named)
{
    std::construct_at(&m_named_type_ptr,
                      std::make_shared<NamedType>(NamedType{name, std::move(type_info)}));
}


TypeInfo& TypeInfo::operator =(const TypeInfo& r)
{
    copy_from_no_key(r);
    m_key = r.m_key;
    return *this;
}


TypeInfo& TypeInfo::operator =(TypeInfo&& r) noexcept {
    assert(this != &r);
    set_type(r.m_type);
    move_variant(std::move(r));
    r.set_type(Type::Unknown);  // NOLINT(bugprone-use-after-move)
    r.m_var = {};
    m_is_literal = r.m_is_literal; r.m_is_literal = true;
    m_key = r.m_key; r.m_key = {};
    return *this;
}


void TypeInfo::copy_from_no_key(const TypeInfo& r) noexcept {
    if (this == &r)
        return;
    set_type(r.m_type);
    copy_variant(r);
    m_is_literal = r.m_is_literal;
}


void TypeInfo::construct_variant() {
    switch (m_type) {
        case Type::Unknown: std::construct_at(&m_var); break;
        case Type::List:
        case Type::Tuple:
        case Type::Struct: std::construct_at(&m_subtypes); break;
        case Type::Function: std::construct_at(&m_signature_ptr); break;
        case Type::Named: std::construct_at(&m_named_type_ptr); break;
        default:
            break;
    }
}


void TypeInfo::destroy_variant() {
    switch (m_type) {
        case Type::Unknown: m_var.~Var(); break;
        case Type::List:
        case Type::Tuple:
        case Type::Struct: m_subtypes.~Subtypes(); break;
        case Type::Function: m_signature_ptr.~SignaturePtr(); break;
        case Type::Named: m_named_type_ptr.~NamedTypePtr(); break;
        default:
            break;
    }
}


void TypeInfo::copy_variant(const xci::script::TypeInfo& r)
{
    switch (m_type) {
        case Type::Unknown: m_var = r.m_var; break;
        case Type::List:
        case Type::Tuple:
        case Type::Struct: m_subtypes = r.m_subtypes; break;
        case Type::Function: m_signature_ptr = r.m_signature_ptr; break;
        case Type::Named: m_named_type_ptr = r.m_named_type_ptr; break;
        default:
            break;
    }
}


void TypeInfo::move_variant(xci::script::TypeInfo&& r)
{
    switch (m_type) {
        case Type::Unknown: m_var = r.m_var; break;
        case Type::List:
        case Type::Tuple:
        case Type::Struct: m_subtypes = std::move(r.m_subtypes); break;
        case Type::Function: m_signature_ptr = std::move(r.m_signature_ptr); break;
        case Type::Named: m_named_type_ptr = std::move(r.m_named_type_ptr); break;
        default:
            break;
    }
}


auto TypeInfo::generic_var() const -> Var
{
    assert(m_type == Type::Unknown);
    return m_var;
}


auto TypeInfo::generic_var() -> Var&
{
    assert(m_type == Type::Unknown);
    return m_var;
}


auto TypeInfo::subtypes() const -> const Subtypes&
{
    assert(m_type == Type::Tuple || m_type == Type::Struct || type() == Type::List);
    return m_subtypes;
}


auto TypeInfo::signature_ptr() const -> const SignaturePtr&
{
    assert(type() == Type::Function);
    return m_signature_ptr;
}


auto TypeInfo::named_type_ptr() const -> const NamedTypePtr&
{
    assert(m_type == Type::Named);
    return m_named_type_ptr;
}


size_t TypeInfo::size() const
{
    switch (type()) {
        case Type::Named:
            return named_type().type_info.size();
        case Type::Tuple:
        case Type::Struct: {
            const auto& l = subtypes();
            return std::accumulate(l.begin(), l.end(), size_t(0),
                              [](size_t init, const TypeInfo& ti) { return init + ti.size(); });
        }
        default:
            return type_size_on_stack(type());
    }
}


void TypeInfo::foreach_heap_slot(std::function<void(size_t offset)> cb) const
{
    const auto& uti = underlying();
    switch (uti.type()) {
        case Type::String:
        case Type::List:
        case Type::Function:
        case Type::Stream:
            cb(0);
            break;
        case Type::Tuple:
        case Type::Struct: {
            size_t pos = 0;
            for (const auto& ti : uti.subtypes()) {
                ti.foreach_heap_slot([&cb, pos](size_t offset) {
                    cb(pos + offset);
                });
                pos += ti.size();
            }
            break;
        }
        default: break;
    }
}


void TypeInfo::replace_var(SymbolPointer var, const TypeInfo& ti)
{
    if (!var)
        return;
    switch (type()) {
        case Type::Unknown:
            if (generic_var() == var)
                *this = ti;
            break;
        case Type::Function:
            if (signature_ptr().use_count() > 1) {
                // multiple users - make copy of the signature
                signature_ptr() = std::make_shared<Signature>(signature());
            }
            signature().param_type.replace_var(var, ti);
            signature().return_type.replace_var(var, ti);
            break;
        case Type::Tuple:
        case Type::Struct:
        case Type::List:
            for (auto& sub : subtypes())
                sub.replace_var(var, ti);
            break;
        default:
            break;
    }
}


const TypeInfo& TypeInfo::effective_type() const
{
    if (is_callable() && !ul_signature().has_nonvoid_param())
        return ul_signature().return_type.effective_type();
    return *this;
}


bool is_same_underlying(const TypeInfo& lhs, const TypeInfo& rhs)
{
    const TypeInfo& l = lhs.underlying();
    const TypeInfo& r = rhs.underlying();
    switch (l.type()) {
        case Type::Unknown:
        case Type::Named:
            return false;
        case Type::Bool:
        case Type::Char:
        case Type::UInt8:
        case Type::UInt16:
        case Type::UInt32:
        case Type::UInt64:
        case Type::UInt128:
        case Type::Int8:
        case Type::Int16:
        case Type::Int32:
        case Type::Int64:
        case Type::Int128:
        case Type::Float32:
        case Type::Float64:
        case Type::Float128:
        case Type::String:
        case Type::Module:
        case Type::Stream:
        case Type::TypeIndex:
            return l.type() == r.type();
        case Type::List:
            return r.type() == Type::List &&
                   is_same_underlying(l.elem_type(), r.elem_type());
        case Type::Tuple:
        case Type::Struct:
            if (r.type() == Type::Tuple || r.type() == Type::Struct) {
                if (l.subtypes().size() != r.subtypes().size())
                    return false;
                auto r_type_it = r.subtypes().begin();
                for (const auto& l_type : l.subtypes()) {
                    if (!is_same_underlying(l_type, *r_type_it++))
                        return false;
                }
                return true;
            }
            return false;
        case Type::Function:
            // FIXME: deep compare underlying types in function prototype?
            return false;
    }
    XCI_UNREACHABLE;
}


bool TypeInfo::operator==(const TypeInfo& rhs) const
{
    if (type() == Type::Unknown || rhs.type() == Type::Unknown)
        return true;  // unknown type matches any other type
    if (type() != rhs.type())
        return false;
    if (key() && rhs.key() && key() != rhs.key())
        return false;  // keys don't match (missing key = match anything)
    switch (type()) {
        case Type::List:
        case Type::Tuple:
        case Type::Struct: return subtypes() == rhs.subtypes();
        case Type::Function: return signature() == rhs.signature();  // compare content, not pointer
        case Type::Named: return named_type() == rhs.named_type();
        default:
            return true;
    }
}


bool TypeInfo::has_unknown() const
{
    switch (type()) {
        case Type::Unknown:
            return true;
        case Type::Function:
            return signature_ptr()->has_any_unknown();
        case Type::List:
            return elem_type().has_unknown();
        case Type::Tuple:
        case Type::Struct:
            return std::ranges::any_of(subtypes(), [](const TypeInfo& type_info) {
                return type_info.has_unknown();
            });
        default:
            return false;
    }
}


bool TypeInfo::has_generic() const
{
    switch (type()) {
        case Type::Unknown:
            return bool(generic_var());
        case Type::Function:
            return signature_ptr()->has_any_generic();
        case Type::List:
            return elem_type().has_generic();
        case Type::Tuple:
        case Type::Struct:
            return std::ranges::any_of(subtypes(), [](const TypeInfo& type_info) {
                return type_info.has_generic();
            });
        default:
            return false;
    }
}


auto TypeInfo::elem_type() const -> const TypeInfo&
{
    assert(type() == Type::List);
    assert(subtypes().size() == 1);
    return subtypes().front();
}


const TypeInfo* TypeInfo::struct_item_by_key(NameId key) const
{
    const auto& items = subtypes();
    auto it = std::find_if(items.begin(), items.end(), [key](const TypeInfo& item) {
         return item.key() == key;
    });
    if (it == items.end())
        return nullptr;
    return &*it;
}


NameId TypeInfo::name() const
{
    return named_type().name;
}


const TypeInfo& TypeInfo::underlying() const
{
    return is_named() ? named_type().type_info.underlying() : *this;
}


TypeInfo& TypeInfo::underlying()
{
    return is_named() ? named_type().type_info.underlying() : *this;
}


bool Signature::has_generic_nonlocals() const
{
    return std::ranges::any_of(nonlocals, [](const TypeInfo& type_info) {
        return type_info.has_generic();
    });
}


bool Signature::has_unknown_nonlocals() const
{
    return std::ranges::any_of(nonlocals, [](const TypeInfo& type_info) {
        return type_info.has_unknown();
    });
}


bool Signature::has_nonvoid_param() const
{
    // Struct can have single Void item
    if (param_type.is_struct()) {
        const auto& items = param_type.subtypes();
        if (items.size() == 1 && items.front().is_void())
            return false;
    }
    // Otherwise, only empty Tuple is considered Void
    return !param_type.is_void();
}


} // namespace xci::script
