// Interpreter.cpp created on 2019-06-21, part of XCI toolkit
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

#include "Interpreter.h"

namespace xci::script {


Interpreter::Interpreter(uint32_t flags)
    : m_compiler(flags)
{
    add_imported_module(BuiltinModule::static_instance());
}


std::unique_ptr<Module> Interpreter::build_module(const std::string& name, std::string_view content)
{
    // setup module
    auto module = std::make_unique<Module>(name);
    module->add_imported_module(BuiltinModule::static_instance());

    // parse
    ast::Module ast;
    m_parser.parse(content, ast);

    // compile
    Function func {*module, module->symtab()};
    m_compiler.compile(func, ast);

    // sanity check (no AST references)
    for (Index idx = 0; idx != module->num_functions(); ++idx) {
        auto& fn = module->get_function(idx);
        if (fn.is_generic()) {
            assert(fn.is_ast_copied());
            fn.ensure_ast_copy();
        }
    }

    return module;
}


std::unique_ptr<Value> Interpreter::eval(std::string_view input, const InvokeCallback& cb)
{
    // parse
    ast::Module ast;
    m_parser.parse(input, ast);

    // compile
    auto& symtab = m_main.symtab().add_child("<input>");
    Function func {m_main, symtab};
    m_compiler.compile(func, ast);

    // execute
    m_machine.call(func, cb);

    // get result from stack
    return m_machine.stack().pull(func.effective_return_type());
}


} // namespace xci::script
