// TypeInfo.cpp created on 2019-06-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "TypeInfo.h"
#include "Error.h"
#include <xci/compat/macros.h>
#include <range/v3/algorithm/any_of.hpp>
#include <numeric>

namespace xci::script {


Type decode_arg_type(uint8_t arg)
{
    switch (arg) {
        case 1: return Type::Byte;
        case 3: return Type::UInt32;
        case 4: return Type::UInt64;
        case 8: return Type::Int32;
        case 9: return Type::Int64;
        case 0xC: return Type::Float32;
        case 0xD: return Type::Float64;
        default: return Type::Unknown;
    }
}


size_t type_size_on_stack(Type type)
{
    switch (type) {
        case Type::Unknown:
        case Type::Void:
        case Type::Tuple:
        case Type::Struct:
        case Type::Named:
            return 0;
        case Type::Bool:
        case Type::Byte:
            return 1;
        case Type::Char:
        case Type::UInt32:
        case Type::Int32:
        case Type::Float32:
            return 4;
        case Type::UInt64:
        case Type::Int64:
        case Type::Float64:
            return 8;
        case Type::String:
        case Type::List:
        case Type::Function:
        case Type::Stream:
        case Type::Module:
            return sizeof(void*);
    }
    return 0;
}


size_t TypeInfo::size() const
{
    switch (m_type) {
        case Type::Named:
            return named_type().type_info.size();
        case Type::Tuple: {
            const auto& l = subtypes();
            return accumulate(l.begin(), l.end(), size_t(0),
                              [](size_t init, const TypeInfo& ti) { return init + ti.size(); });
        }
        case Type::Struct: {
            const auto& l = struct_items();
            return accumulate(l.begin(), l.end(), size_t(0),
                              [](size_t init, const TypeInfo::StructItem& ti) { return init + ti.second.size(); });
        }
        default:
            return type_size_on_stack(type());
    }
}


void TypeInfo::foreach_heap_slot(std::function<void(size_t offset)> cb) const
{
    switch (underlying_type()) {
        case Type::String:
        case Type::List:
        case Type::Function:
        case Type::Stream:
            cb(0);
            break;
        case Type::Tuple: {
            size_t pos = 0;
            for (const auto& ti : subtypes()) {
                ti.foreach_heap_slot([&cb, pos](size_t offset) {
                    cb(pos + offset);
                });
                pos += ti.size();
            }
            break;
        }
        case Type::Struct: {
            size_t pos = 0;
            for (const auto& item : struct_items()) {
                item.second.foreach_heap_slot([&cb, pos](size_t offset) {
                    cb(pos + offset);
                });
                pos += item.second.size();
            }
            break;
        }
        default: break;
    }
}


void TypeInfo::replace_var(uint8_t idx, const TypeInfo& ti)
{
    if (idx == 0)
        return;
    switch (m_type) {
        case Type::Unknown:
            if (generic_var() == idx)
                *this = ti;
            break;
        case Type::Function: {
            SignaturePtr sig_ptr;
            if (signature_ptr().use_count() > 1) {
                // multiple users - make copy of the signature
                m_info = std::make_shared<Signature>(signature());
            }
            for (auto& prm : signature().params) {
                prm.replace_var(idx, ti);
            }
            signature().return_type.replace_var(idx, ti);
            break;
        }
        case Type::Tuple:
        case Type::List:
            assert(std::holds_alternative<Subtypes>(m_info));
            for (auto& sub : std::get<Subtypes>(m_info))
                sub.replace_var(idx, ti);
            break;
        case Type::Struct:
            assert(std::holds_alternative<StructItems>(m_info));
            for (auto& item : std::get<StructItems>(m_info))
                item.second.replace_var(idx, ti);
            break;
        default:
            break;
    }
}


TypeInfo::TypeInfo(Type type) : m_type(type)
{
    switch (type) {
        case Type::Unknown:
            m_info = Var(0);
            break;
        case Type::List:
        case Type::Tuple:
            m_info = Subtypes();
            break;
        case Type::Struct:
            m_info = StructItems();
            break;
        case Type::Function:
            m_info = SignaturePtr();
            break;
        default:
            break;
    }
}


TypeInfo::TypeInfo(ListTag, TypeInfo list_elem)
        : m_type(Type::List), m_info(std::vector{std::move(list_elem)})
{}


TypeInfo::TypeInfo(std::string name, TypeInfo&& type_info)
        : m_type(Type::Named),
          m_info(std::make_shared<NamedType>(NamedType{std::move(name), std::move(type_info)}))
{}


TypeInfo::TypeInfo(TypeInfo&& other) noexcept
        : m_type(other.m_type), m_info(std::move(other.m_info))
{
    other.m_type = Type::Unknown;
    other.m_info = Var{};
}


TypeInfo& TypeInfo::operator=(TypeInfo&& other) noexcept
{
    m_type = other.m_type;
    m_info = std::move(other.m_info);
    other.m_type = Type::Unknown;
    other.m_info = Var{};
    return *this;
}


const TypeInfo& TypeInfo::effective_type() const
{
    if (is_callable() && !signature().has_nonvoid_params())
        return signature().return_type.effective_type();
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
        case Type::Void:
        case Type::Bool:
        case Type::Byte:
        case Type::Char:
        case Type::UInt32:
        case Type::UInt64:
        case Type::Int32:
        case Type::Int64:
        case Type::Float32:
        case Type::Float64:
        case Type::String:
        case Type::Module:
        case Type::Stream:
            return l.type() == r.type();
        case Type::List:
            return r.type() == Type::List &&
                   is_same_underlying(l.elem_type(), r.elem_type());
        case Type::Tuple:
        case Type::Struct:
            if (r.type() == Type::Tuple || r.type() == Type::Struct) {
                auto l_types = l.struct_or_tuple_subtypes();
                auto r_types = r.struct_or_tuple_subtypes();
                if (l_types.size() != r_types.size())
                    return false;
                auto r_type_it = r_types.begin();
                for (const auto& l_type : l_types) {
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
    UNREACHABLE;
}


bool TypeInfo::operator==(const TypeInfo& rhs) const
{
    if (m_type == Type::Unknown || rhs.type() == Type::Unknown)
        return true;  // unknown type matches any other type
    if (m_type != rhs.type())
        return false;
    if (m_type == Type::Function)
        return signature() == rhs.signature();  // compare content, not pointer
    if (m_type == Type::Named)
        return named_type() == rhs.named_type();
    return m_info == rhs.m_info;
}


Type TypeInfo::underlying_type() const
{
    return m_type == Type::Named ? named_type().type_info.underlying_type() : m_type;
}


bool TypeInfo::is_generic() const
{
    if (m_type == Type::Unknown)
        return true;
    if (m_type == Type::Function)
        return signature_ptr()->is_generic();
    if (m_type == Type::List)
        return elem_type().is_generic();
    if (m_type == Type::Tuple)
        return ranges::any_of(subtypes(), [](const TypeInfo& type_info) {
            return type_info.is_generic();
        });
    return false;
}


auto TypeInfo::generic_var() const -> Var
{
    assert(m_type == Type::Unknown);
    assert(std::holds_alternative<Var>(m_info));
    return std::get<Var>(m_info);
}


auto TypeInfo::elem_type() const -> const TypeInfo&
{
    if (m_type == Type::Named)
        return named_type().type_info.elem_type();
    assert(m_type == Type::List);
    assert(std::holds_alternative<Subtypes>(m_info));
    return std::get<Subtypes>(m_info)[0];
}


auto TypeInfo::subtypes() const -> const Subtypes&
{
    if (m_type == Type::Named)
        return named_type().type_info.subtypes();
    assert(m_type == Type::Tuple);
    assert(std::holds_alternative<Subtypes>(m_info));
    return std::get<Subtypes>(m_info);
}


auto TypeInfo::struct_items() const -> const StructItems&
{
    if (m_type == Type::Named)
        return named_type().type_info.struct_items();
    assert(m_type == Type::Struct);
    assert(std::holds_alternative<StructItems>(m_info));
    return std::get<StructItems>(m_info);
}


const TypeInfo* TypeInfo::struct_item_by_name(const std::string& name) const
{
    const auto& items = struct_items();
    auto it = std::find_if(items.begin(), items.end(), [&name](const StructItem& item) {
         return item.first == name;
    });
    if (it == items.end())
        return nullptr;
    return &it->second;
}


auto TypeInfo::struct_or_tuple_subtypes() const -> Subtypes
{
    if (m_type == Type::Tuple)
        return subtypes();
    const auto& items = struct_items();
    Subtypes res;
    res.reserve(items.size());
    std::transform(items.begin(), items.end(), std::back_inserter(res), [](const auto& item) {
        return item.second;
    });
    return res;
}


auto TypeInfo::signature_ptr() const -> const SignaturePtr&
{
    if (m_type == Type::Named)
        return named_type().type_info.signature_ptr();
    assert(m_type == Type::Function);
    assert(std::holds_alternative<SignaturePtr>(m_info));
    return std::get<SignaturePtr>(m_info);
}


const TypeInfo::NamedTypePtr& TypeInfo::named_type_ptr() const
{
    assert(m_type == Type::Named);
    assert(std::holds_alternative<NamedTypePtr>(m_info));
    return std::get<NamedTypePtr>(m_info);
}


const TypeInfo& TypeInfo::underlying() const
{
    return m_type == Type::Named ? named_type().type_info.underlying() : *this;
}


std::string TypeInfo::name() const
{
    return named_type().name;
}


bool Signature::has_generic_params() const
{
    return ranges::any_of(params, [](const TypeInfo& type_info) {
        return type_info.is_generic();
    });
}


bool Signature::has_nonvoid_params() const
{
    return ranges::any_of(params, [](const TypeInfo& type_info) {
        return !type_info.is_void();
    });
}


} // namespace xci::script
