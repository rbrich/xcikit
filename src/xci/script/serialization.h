// serialization.h created on 2026-11-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2026 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_SERIALIZATION_H
#define XCI_SCRIPT_SERIALIZATION_H

#include "TypeInfo.h"
#include "Value.h"

namespace xci::script {


// TypeInfo

template <class Archive>
void save_schema(Archive& ar, const TypeInfo& v) {
    ar("key", v.key());
    ar("type", v.type());
    if (ar.enter_union("info", "type", typeid(TypeInfo))) {
        ar(uint8_t(Type::Unknown), "var", SymbolPointer{});
        ar(uint8_t(Type::List), "elem_type", TypeInfo{});
        ar(uint8_t(Type::Tuple), "subtypes", TypeInfo::Subtypes{});
        ar(uint8_t(Type::Function), "signature", SignaturePtr{});
        ar(uint8_t(Type::Named), "named_type", SymbolPointer{});
        ar(uint8_t(Type::Struct), "subtypes", TypeInfo::Subtypes{});
        ar.leave_union();
    }
    ar("default", v.default_value().get());
}

template <class Archive>
void save(Archive& ar, const TypeInfo& v) {
    if (v.key())
        ar("key", v.key());
    ar("type", v.type());
    switch (v.type()) {
        case Type::Unknown:
            ar("var", v.symptr());
            break;
        case Type::Function:
            ar("signature", v.signature());
            break;
        case Type::List:
            ar("elem_type", v.elem_type());
            break;
        case Type::Tuple:
        case Type::Struct:
            ar("subtypes", v.subtypes());
            break;
        case Type::Named:
            ar("named_type", v.symptr());
            break;
        default:
            break;
    }
    ar("default", v.default_value().get());
}

template <class Archive>
void load(Archive& ar, TypeInfo& v)
{
    NameId key;
    ar("key", key);
    v.set_key(key);

    Type t;
    ar("type", t);
    v.set_type(t);

    switch (t) {
        case Type::Unknown: {
            ar("var", v.symptr());
            break;
        }
        case Type::Function: {
            ar("signature", *v.signature_ptr());
            break;
        }
        case Type::List: {
            v.subtypes().reset(1);
            ar("elem_type", v.subtypes().front());
            break;
        }
        case Type::Tuple:
        case Type::Struct: {
            std::vector<TypeInfo> items;
            ar("subtypes", items);
            v.subtypes() = TypeInfo::Subtypes(core::to_span(items));
            break;
        }
        case Type::Named: {
            ar("named_type", v.symptr());
            break;
        }
        default:
            break;
    }

    Value default_value;
    ar("default", default_value);
    if (!default_value.is_unknown())
        v.set_default_value(ValueId::add(std::move(default_value)));
}


} // namespace xci::script

#endif // include guard
