// Function.cpp created on 2019-05-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Function.h"
#include "Module.h"
#include "Error.h"
#include <range/v3/algorithm/any_of.hpp>
#include <utility>
#include <numeric>

namespace xci::script {

using ranges::cpp20::any_of;


Function::Function(Module& module, SymbolTable& symtab)
      : m_module(module), m_symtab(symtab),
        m_signature(std::make_shared<Signature>())
{
    symtab.set_function(this);
}


void Function::add_parameter(std::string name, TypeInfo&& type_info)
{
    m_symtab.add({std::move(name), Symbol::Parameter, parameters().size()});
    signature().add_parameter(std::move(type_info));
}


size_t Function::raw_size_of_parameters() const
{
    return std::accumulate(m_signature->params.begin(), m_signature->params.end(), size_t(0),
               [](size_t init, const TypeInfo& ti) { return init + ti.size(); });
}


size_t Function::parameter_offset(Index idx) const
{
    size_t ofs = 0;
    for (const auto& ti : m_signature->params) {
        if (idx == 0)
            return ofs;
        ofs += ti.size();
        idx --;
    }
    assert(!"parameter index out of range");
    return 0;
}


void Function::add_nonlocal(TypeInfo&& type_info)
{
    signature().add_nonlocal(std::move(type_info));
}


size_t Function::raw_size_of_nonlocals() const
{
    return std::accumulate(nonlocals().begin(), nonlocals().end(), size_t(0),
            [](size_t init, const TypeInfo& ti) { return init + ti.size(); });
}


std::pair<size_t, TypeInfo> Function::nonlocal_offset_and_type(Index idx) const
{
    size_t ofs = 0;
    for (const auto& ti : nonlocals()) {
        if (idx == 0)
            return {ofs, ti};
        ofs += ti.size();
        idx --;
    }
    assert(!"nonlocal index out of range");
    return {0, {}};
}


void Function::add_partial(TypeInfo&& type_info)
{
    signature().add_partial(std::move(type_info));
}


size_t Function::raw_size_of_partial() const
{
    return std::accumulate(partial().begin(), partial().end(), size_t(0),
            [](size_t init, const TypeInfo& ti) { return init + ti.size(); });
}


std::vector<TypeInfo> Function::closure() const
{
    auto closure = nonlocals();
    std::copy(partial().cbegin(), partial().cend(), std::back_inserter(closure));
    return closure;
}


bool Function::detect_generic() const
{
    return any_of(signature().params, [](const TypeInfo& type_info) {
        return type_info.type() == Type::Unknown;
    });
}


bool Function::operator==(const Function& rhs) const
{
    return &m_module == &rhs.m_module &&
           &m_symtab == &rhs.m_symtab &&
           *m_signature == *rhs.m_signature &&
           m_body == rhs.m_body;
}


bool Function::CompiledBody::operator==(const Function::CompiledBody& rhs) const {
    return code == rhs.code &&
           is_fragment == rhs.is_fragment;
}


bool Function::GenericBody::operator==(const Function::GenericBody& rhs) const
{
    return false;
}


bool Function::NativeBody::operator==(const Function::NativeBody& rhs) const
{
    return native == rhs.native;
}


} // namespace xci::script
