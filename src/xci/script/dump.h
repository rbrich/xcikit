// dump.h created on 2019-10-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_DUMP_H
#define XCI_SCRIPT_DUMP_H

#include "ast/AST.h"
#include "SymbolTable.h"
#include "TypeInfo.h"
#include "Function.h"
#include <ostream>

namespace xci::script {

// stream manipulators
std::ostream& dump_tree(std::ostream& os);
std::ostream& dump_module_verbose(std::ostream& os);
std::ostream& put_indent(std::ostream& os);
std::ostream& rule_indent(std::ostream& os);  // indent with visible guide
std::ostream& more_indent(std::ostream& os);
std::ostream& less_indent(std::ostream& os);

// AST
namespace ast {
std::ostream& operator<<(std::ostream& os, const Literal& v);
std::ostream& operator<<(std::ostream& os, const Parenthesized& v);
std::ostream& operator<<(std::ostream& os, const Tuple& v);
std::ostream& operator<<(std::ostream& os, const List& v);
std::ostream& operator<<(std::ostream& os, const StructInit& v);
std::ostream& operator<<(std::ostream& os, const Variable& v);
std::ostream& operator<<(std::ostream& os, const StructItem& v);
std::ostream& operator<<(std::ostream& os, const Parameter& v);
std::ostream& operator<<(std::ostream& os, const Identifier& v);
std::ostream& operator<<(std::ostream& os, const Type& v);
std::ostream& operator<<(std::ostream& os, const TypeName& v);
std::ostream& operator<<(std::ostream& os, const FunctionType& v);
std::ostream& operator<<(std::ostream& os, const ListType& v);
std::ostream& operator<<(std::ostream& os, const TupleType& v);
std::ostream& operator<<(std::ostream& os, const StructType& v);
std::ostream& operator<<(std::ostream& os, const TypeConstraint& v);
std::ostream& operator<<(std::ostream& os, const Reference& v);
std::ostream& operator<<(std::ostream& os, const Call& v);
std::ostream& operator<<(std::ostream& os, const OpCall& v);
std::ostream& operator<<(std::ostream& os, const Condition& v);
std::ostream& operator<<(std::ostream& os, const WithContext& v);
std::ostream& operator<<(std::ostream& os, const Operator& v);
std::ostream& operator<<(std::ostream& os, const Function& v);
std::ostream& operator<<(std::ostream& os, const Cast& v);
std::ostream& operator<<(std::ostream& os, const Expression& v);
std::ostream& operator<<(std::ostream& os, const Definition& v);
std::ostream& operator<<(std::ostream& os, const Invocation& v);
std::ostream& operator<<(std::ostream& os, const Return& v);
std::ostream& operator<<(std::ostream& os, const Class& v);
std::ostream& operator<<(std::ostream& os, const Instance& v);
std::ostream& operator<<(std::ostream& os, const TypeDef& v);
std::ostream& operator<<(std::ostream& os, const TypeAlias& v);
std::ostream& operator<<(std::ostream& os, const Block& v);
std::ostream& operator<<(std::ostream& os, const Module& v);
} // namespace ast

// Function
std::ostream& operator<<(std::ostream& os, const Function& f);
std::ostream& operator<<(std::ostream& os, Function::Kind v);
struct DumpInstruction { const Function& func; const CodeAssembly::Instruction& instr; };
std::ostream& operator<<(std::ostream& os, DumpInstruction&& v);
struct DumpBytecode { const Function& func; Code::const_iterator& pos; };
std::ostream& operator<<(std::ostream& os, DumpBytecode&& v);

// Module
std::ostream& operator<<(std::ostream& os, const Module& v);

// Type
std::ostream& operator<<(std::ostream& os, const TypeInfo& v);
std::ostream& operator<<(std::ostream& os, const Signature& v);

// SymbolTable
std::ostream& operator<<(std::ostream& os, Symbol::Type v);
std::ostream& operator<<(std::ostream& os, const Symbol& v);
std::ostream& operator<<(std::ostream& os, const SymbolPointer& v);
std::ostream& operator<<(std::ostream& os, const SymbolTable& v);

// Scopes
std::ostream& operator<<(std::ostream& os, const Scope& v);
std::ostream& operator<<(std::ostream& os, const TypeArgs& v);

} // namespace xci::script

template <> struct fmt::formatter<xci::script::TypeInfo> : ostream_formatter {};
template <> struct fmt::formatter<xci::script::Signature> : ostream_formatter {};
template <> struct fmt::formatter<xci::script::TypeArgs> : ostream_formatter {};


#endif // include guard
