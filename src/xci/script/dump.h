// dump.h created on 2019-10-08, part of XCI toolkit
// Copyright 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef XCI_SCRIPT_DUMP_H
#define XCI_SCRIPT_DUMP_H

#include "AST.h"
#include "SymbolTable.h"
#include "TypeInfo.h"
#include "Function.h"
#include <ostream>

namespace xci::script {

// stream manipulators
std::ostream& dump_tree(std::ostream& os);
std::ostream& put_indent(std::ostream& os);
std::ostream& more_indent(std::ostream& os);
std::ostream& less_indent(std::ostream& os);

// AST
namespace ast {
std::ostream& operator<<(std::ostream& os, const Integer& v);
std::ostream& operator<<(std::ostream& os, const Float& v);
std::ostream& operator<<(std::ostream& os, const String& v);
std::ostream& operator<<(std::ostream& os, const Tuple& v);
std::ostream& operator<<(std::ostream& os, const List& v);
std::ostream& operator<<(std::ostream& os, const Variable& v);
std::ostream& operator<<(std::ostream& os, const Parameter& v);
std::ostream& operator<<(std::ostream& os, const Identifier& v);
std::ostream& operator<<(std::ostream& os, const Type& v);
std::ostream& operator<<(std::ostream& os, const TypeName& v);
std::ostream& operator<<(std::ostream& os, const FunctionType& v);
std::ostream& operator<<(std::ostream& os, const ListType& v);
std::ostream& operator<<(std::ostream& os, const TypeConstraint& v);
std::ostream& operator<<(std::ostream& os, const Reference& v);
std::ostream& operator<<(std::ostream& os, const Call& v);
std::ostream& operator<<(std::ostream& os, const OpCall& v);
std::ostream& operator<<(std::ostream& os, const Condition& v);
std::ostream& operator<<(std::ostream& os, const Operator& v);
std::ostream& operator<<(std::ostream& os, const Function& v);
std::ostream& operator<<(std::ostream& os, const Expression& v);
std::ostream& operator<<(std::ostream& os, const Definition& v);
std::ostream& operator<<(std::ostream& os, const Invocation& v);
std::ostream& operator<<(std::ostream& os, const Return& v);
std::ostream& operator<<(std::ostream& os, const Class& v);
std::ostream& operator<<(std::ostream& os, const Instance& v);
std::ostream& operator<<(std::ostream& os, const Block& v);
std::ostream& operator<<(std::ostream& os, const Module& v);
} // namespace ast

// Function
std::ostream& operator<<(std::ostream& os, const Function& f);
std::ostream& operator<<(std::ostream& os, Function::Kind v);
struct DumpInstruction { const Function& func; Code::const_iterator& pos; };
std::ostream& operator<<(std::ostream& os, DumpInstruction&& v);

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

} // namespace xci::script

#endif // include guard
