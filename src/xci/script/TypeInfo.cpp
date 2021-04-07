// TypeInfo.cpp created on 2019-06-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "TypeInfo.h"
#include "Error.h"
#include <numeric>

namespace xci::script {


Type decode_arg_type(uint8_t arg)
{
    switch (arg) {
        case 1: return Type::Byte;
        case 8: return Type::Int32;
        case 9: return Type::Int64;
        case 0xC: return Type::Float32;
        case 0xD: return Type::Float64;
        default: return Type::Unknown;
    }
}


size_t TypeInfo::size() const
{
    switch (type()) {
        case Type::Unknown:     return 0;
        case Type::Void:        return 0;
        case Type::Bool:        return 1;
        case Type::Byte:        return 1;
        case Type::Char:        return 4;
        case Type::Int32:       return 4;
        case Type::Int64:       return 8;
        case Type::Float32:     return 4;
        case Type::Float64:     return 8;

        case Type::String:
        case Type::List:
            return sizeof(byte*) + sizeof(size_t);

        case Type::Tuple:
            return accumulate(subtypes().begin(), subtypes().end(), size_t(0),
                              [](size_t init, const TypeInfo& ti)
                              { return init + ti.size(); });

        case Type::Function:    return sizeof(byte*) + sizeof(void*);
        case Type::Module:      return 0;  // TODO
    }
    return 0;
}


void TypeInfo::foreach_heap_slot(std::function<void(size_t offset)> cb) const
{
    switch (type()) {
        case Type::String:      cb(0); break;
        case Type::Function:    cb(0); break;
        case Type::List:        cb(0); break;
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
            // work on copy of signature
            auto sig_copy = std::make_shared<Signature>(signature());
            for (auto& prm : sig_copy->params) {
                prm.replace_var(idx, ti);
            }
            sig_copy->return_type.replace_var(idx, ti);
            m_info = move(sig_copy);
            break;
        }
        case Type::Tuple:
        case Type::List:
            assert(std::holds_alternative<Subtypes>(m_info));
            for (auto& sub : std::get<Subtypes>(m_info))
                sub.replace_var(idx, ti);
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
        case Type::Function:
            m_info = SignaturePtr();
            break;
        default:
            break;
    }
}


TypeInfo::TypeInfo(Type type, TypeInfo list_elem)
        : m_type(Type::List), m_info(std::vector{std::move(list_elem)})
{
    assert(type == Type::List);
}


TypeInfo TypeInfo::effective_type() const
{
    if (is_callable() && signature().params.empty())
        return signature().return_type.effective_type();
    return *this;
}


auto TypeInfo::generic_var() const -> Var
{
    assert(m_type == Type::Unknown);
    assert(std::holds_alternative<Var>(m_info));
    return std::get<Var>(m_info);
}


auto TypeInfo::elem_type() const -> const TypeInfo&
{
    assert(m_type == Type::List);
    assert(std::holds_alternative<Subtypes>(m_info));
    return std::get<Subtypes>(m_info)[0];
}


auto TypeInfo::subtypes() const -> const Subtypes&
{
    assert(m_type == Type::Tuple);
    assert(std::holds_alternative<Subtypes>(m_info));
    return std::get<Subtypes>(m_info);
}


auto TypeInfo::signature_ptr() const -> const SignaturePtr&
{
    assert(m_type == Type::Function);
    assert(std::holds_alternative<SignaturePtr>(m_info));
    return std::get<SignaturePtr>(m_info);
}


bool TypeInfo::operator==(const TypeInfo& rhs) const
{
    if (m_type == Type::Unknown || rhs.type() == Type::Unknown)
        return true;  // unknown type matches all other types
    if (m_type != rhs.type())
        return false;
    if (m_type == Type::Function)
        return signature() == rhs.signature();  // compare content, not pointer
    return m_info == rhs.m_info;
}


void Signature::resolve_return_type(const TypeInfo& t)
{
    if (!return_type) {
        if (!t)
            throw MissingExplicitType();
        return_type = t;
        return;
    }
    if (return_type != t)
        throw UnexpectedReturnType(return_type, t);
}


} // namespace xci::script
