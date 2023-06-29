// AST_serialization.h created on 2022-01-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_AST_SERIALIZATION_H
#define XCI_SCRIPT_AST_SERIALIZATION_H

#include "AST.h"

namespace xci::script::ast {


template <class Archive>
void serialize(Archive& ar, Block& v)
{
    // TODO: AST serialization
    //ar(v.statements);
}


template <class Archive>
void save(Archive& ar, const Expression& v)
{
    // TODO: AST serialization
}

//template <class Archive>
//void load(Archive& ar, std::unique_ptr<ast::Expression>& v)
//{
//    // TODO: AST serialization
//}


} // namespace xci::script::ast

#endif // include guard
