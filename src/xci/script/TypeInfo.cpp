// TypeInfo.cpp created on 2019-06-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "TypeInfo.h"
#include "Error.h"
#include <numeric>

namespace xci::script {


size_t TypeInfo::size() const
{
    switch (type()) {
        case Type::Unknown:     return 0;
        case Type::Void:        return 1;
        case Type::Bool:        return 1;
        case Type::Byte:        return 1;
        case Type::Char:        return 4;
        case Type::Int32:       return 4;
        case Type::Int64:       return 8;
        case Type::Float32:     return 4;
        case Type::Float64:     return 8;
        case Type::String:      return sizeof(byte*) + sizeof(size_t);
        case Type::List:        return sizeof(byte*) + sizeof(size_t);
        case Type::Tuple:
            return accumulate(m_subtypes.begin(), m_subtypes.end(), size_t(0),
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
            for (const auto& ti : m_subtypes) {
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
            if (m_var == idx)
                *this = ti;
            break;
        case Type::Function: {
            // work on copy of signature
            auto sig_copy = std::make_shared<Signature>(*m_signature);
            for (auto& prm : sig_copy->params) {
                prm.replace_var(idx, ti);
            }
            sig_copy->return_type.replace_var(idx, ti);
            m_signature = move(sig_copy);
            break;
        }
        case Type::Tuple:
        case Type::List:
            for (auto& sub : m_subtypes)
                sub.replace_var(idx, ti);
            break;
        default:
            break;
    }
}


TypeInfo TypeInfo::effective_type() const
{
    if (is_callable() && signature().params.empty())
        return signature().return_type.effective_type();
    return *this;
}


const TypeInfo& TypeInfo::elem_type() const
{
    assert(m_type == Type::List);
    return m_subtypes[0];
}


bool TypeInfo::operator==(const TypeInfo& rhs) const
{
    if (m_type == Type::Unknown || rhs.type() == Type::Unknown)
        return true;  // unknown type matches all other types
    if (m_type != rhs.type())
        return false;
    if (m_type == Type::Function)
        return *m_signature == *rhs.m_signature;
    if (m_type == Type::Tuple || m_type == Type::List)
        return m_subtypes == rhs.m_subtypes;
    return true;
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
