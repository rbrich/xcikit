// Builtin.h created on 2019-05-20, part of XCI toolkit
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

#ifndef XCI_SCRIPT_BUILTIN_H
#define XCI_SCRIPT_BUILTIN_H

#include "Value.h"
#include "Module.h"
#include "ast/AST.h"
#include "Code.h"
#include <functional>

namespace xci::script {


namespace builtin {
    template <class T, class R=T> using BinaryFunction = std::function<R(T, T)>;
    template <class T> using UnaryFunction = std::function<T(T)>;

    BinaryFunction<value::Bool> logical_op_function(Opcode opcode);
    template <class T> BinaryFunction<T, value::Bool> comparison_op_function(Opcode opcode);
    template <class T> BinaryFunction<T> binary_op_function(Opcode opcode);

    UnaryFunction<value::Bool> logical_not_function();
    template <class T> UnaryFunction<T> unary_op_function(Opcode opcode);

    const char* op_to_name(ast::Operator::Op op);
    const char* op_to_function_name(ast::Operator::Op op);
}


class BuiltinModule : public Module {
public:
    BuiltinModule();

    static BuiltinModule& static_instance();

private:
    void add_logical_op_function(const std::string& name, Opcode opcode);
    void add_bitwise_op_function(const std::string& name, Opcode opcode);
    void add_arithmetic_op_function(const std::string& name, Opcode opcode);
    void add_unary_op_functions();
    void add_subscript_function();
    void add_intrinsics();
    void add_types();
};

} // namespace xci::script

#endif // include guard
