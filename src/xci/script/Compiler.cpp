// Compiler.cpp created on 2019-05-30, part of XCI toolkit
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

#include "Compiler.h"
#include "Builtin.h"
#include "Error.h"
#include "Optimizer.h"
#include "SymbolResolver.h"
#include "TypeResolver.h"
#include "Stack.h"
#include <xci/core/format.h>
#include <xci/compat/macros.h>

#include <range/v3/view/reverse.hpp>
#include <range/v3/action/reverse.hpp>

#include <stack>
#include <sstream>
#include <cassert>

namespace xci::script {

using namespace std;


class CompilerVisitor: public ast::Visitor {
public:
    explicit CompilerVisitor(Compiler& compiler, Function& function)
        : m_compiler(compiler), m_function(function) {}

    void visit(ast::Definition& dfn) override {
        dfn.expression->apply(*this);
        // local value is on stack
    }

    void visit(ast::Invocation& inv) override {
        inv.expression->apply(*this);
        m_function.code().add_opcode(Opcode::Invoke);
    }

    void visit(ast::Return& ret) override {
        ret.expression->apply(*this);
        // if the return value is parameterless function, evaluate it
        TypeInfo& rti = m_function.signature().return_type;
        if (rti.is_callable() && rti.signature().params.empty()) {
            // EXECUTE
            m_function.code().add_opcode(Opcode::Execute);
            rti = rti.signature().return_type;
        }

        auto skip = rti.size();
        auto drop = m_function.raw_size_of_parameters()
                  + m_function.raw_size_of_nonlocals()
                  + m_function.raw_size_of_values();
        if (drop > 0) {
            Stack::StackRel pos = skip;
            for (const auto& ti : ranges::views::reverse(m_function.values())) {
                ti.foreach_heap_slot([this, pos](size_t offset) {
                    // DEC_REF <addr in locals>
                    m_function.code().add_opcode(Opcode::DecRef, pos + offset);
                });
                pos += ti.size();
            }
            for (const auto& ti : m_function.nonlocals() | ranges::actions::reverse) {
                ti.foreach_heap_slot([this, pos](size_t offset) {
                    // DEC_REF <addr in nonlocals>
                    m_function.code().add_opcode(Opcode::DecRef, pos + offset);
                });
                pos += ti.size();
            }
            for (const auto& ti : ranges::views::reverse(m_function.parameters())) {
                ti.foreach_heap_slot([this, pos](size_t offset) {
                    // DEC_REF <addr in params>
                    m_function.code().add_opcode(Opcode::DecRef, pos + offset);
                });
                pos += ti.size();
            }
            // DROP <ret_value> <params + locals + nonlocals>
            m_function.code().add_opcode(Opcode::Drop, skip, drop);
        }
        // return value left on stack
    }

    void visit(ast::Integer& v) override {
        // add to static values
        auto idx = module().add_value(std::make_unique<value::Int32>(v.value));
        // LOAD_STATIC <static_idx>
        m_function.code().add_opcode(Opcode::LoadStatic, idx);
    }

    void visit(ast::Float& v) override {
        // add to static values
        auto idx = module().add_value(std::make_unique<value::Float32>(v.value));
        // LOAD_STATIC <static_idx>
        m_function.code().add_opcode(Opcode::LoadStatic, idx);
    }

    void visit(ast::String& v) override {
        // add to static values
        auto idx = module().add_value(std::make_unique<value::String>(v.value));
        // LOAD_STATIC <static_idx>
        m_function.code().add_opcode(Opcode::LoadStatic, idx);
    }

    void visit(ast::Tuple& v) override {
        // build tuple on stack
        for (auto& item : ranges::views::reverse(v.items)) {
            item->apply(*this);
        }
    }

    void visit(ast::Call& v) override {
        // evaluate each arg
        for (auto& arg : ranges::views::reverse(v.args)) {
            arg->apply(*this);
        }
        // call the function or push the value
        assert(v.identifier.symbol);
        auto& symtab = *v.identifier.symbol.symtab();
        auto& sym = *v.identifier.symbol;
        switch (sym.type()) {
            case Symbol::Module: {
                assert(sym.depth() == 0);
                // LOAD_MODULE <module_idx>
                m_function.code().add_opcode(Opcode::LoadModule, sym.index());
                break;
            }
            case Symbol::Nonlocal: {
                // Non-locals are captured in closure - read from closure
                auto ofs_ti = m_function.nonlocal_offset_and_type(sym.index());
                // COPY_ARGUMENT <frame_offset>
                m_function.code().add_opcode(Opcode::CopyArgument,
                    ofs_ti.first, ofs_ti.second.size());
                ofs_ti.second.foreach_heap_slot([this](size_t offset) {
                    // INC_REF <stack_offset>
                    m_function.code().add_opcode(Opcode::IncRef, offset);
                });
                break;
            }
            case Symbol::Value: {
                if (symtab.module() != nullptr) {
                    // static value
                    auto idx = sym.index();
                    if (symtab.module() != &module()) {
                        // copy static value into this module if it's from builtin or another module
                        auto& val = symtab.module()->get_value(sym.index());
                        idx = module().add_value(val.make_copy());
                    }
                    // LOAD_STATIC <static_idx>
                    m_function.code().add_opcode(Opcode::LoadStatic, idx);
                    break;
                }
                assert(sym.depth() == 0);
                // COPY_VARIABLE <frame_offset> <size>
                const auto& ti = m_function.get_value(sym.index());
                m_function.code().add_opcode(Opcode::CopyVariable,
                    m_function.value_offset(sym.index()),
                    ti.size());
                ti.foreach_heap_slot([this](size_t offset) {
                    m_function.code().add_opcode(Opcode::IncRef, offset);
                });
                break;
            }
            case Symbol::Parameter: {
                assert(sym.depth() == 0);
                // COPY_ARGUMENT <frame_offset> <size>
                auto nonlocals_size = m_function.raw_size_of_nonlocals();
                const auto& ti = m_function.get_parameter(sym.index());
                m_function.code().add_opcode(Opcode::CopyArgument,
                    m_function.parameter_offset(sym.index()) + nonlocals_size,
                    ti.size());
                ti.foreach_heap_slot([this](size_t offset) {
                    m_function.code().add_opcode(Opcode::IncRef, offset);
                });
                break;
            }
            case Symbol::Function: {
                // this module
                if (symtab.module() == nullptr || symtab.module() == &m_function.module()) {
                    // CALL0 <function_idx>
                    m_function.code().add_opcode(Opcode::Call0, sym.index());
                    break;
                }
                // builtin module or imported module
                auto mod_idx = m_function.module().get_imported_module_index(symtab.module());
                assert(mod_idx != no_index);
                if (mod_idx == 0) {
                    // CALL1 <function_idx>
                    m_function.code().add_opcode(Opcode::Call1, sym.index());
                } else {
                    // CALL <module_idx> <function_idx>
                    m_function.code().add_opcode(Opcode::Call, mod_idx, sym.index());
                }
                break;
            }
            case Symbol::Unresolved:
                UNREACHABLE;
        }
        // if it's function object, execute it
        assert(v.args.empty() || sym.is_callable());
        if (sym.type() != Symbol::Function && sym.is_callable()) {
            m_function.code().add_opcode(Opcode::Execute);
        }
    }

    void visit(ast::OpCall& v) override {
        assert(!v.right_tmp);
        v.identifier.name = builtin::op_to_function_name(v.op.op);
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        // evaluate condition
        v.cond->apply(*this);
        // add jump instruction
        m_function.code().add_opcode(Opcode::JumpIfNot, 0);
        auto jump1_arg_pos = m_function.code().this_instruction_address();
        // then branch
        v.then_expr->apply(*this);
        m_function.code().add_opcode(Opcode::Jump, 0);
        auto jump2_arg_pos = m_function.code().this_instruction_address();
        // else branch
        m_function.code().set_arg(jump1_arg_pos,
                m_function.code().this_instruction_address() - jump1_arg_pos);
        v.else_expr->apply(*this);
        // end
        m_function.code().set_arg(jump2_arg_pos,
                m_function.code().this_instruction_address() - jump2_arg_pos);
    }

    void visit(ast::Function& v) override {
        // compile body
        Function& func = m_compiler.module().get_function(v.index);
        m_compiler.compile_block(func, v.body);
        assert(func.symtab().parent() == &m_function.symtab());
        auto n_nonlocals = func.symtab().count_nonlocals();
        auto nonlocals_size = m_function.raw_size_of_nonlocals();
        if (n_nonlocals > 0) {
            // make closure
            for (const auto& sym : func.symtab()) {
                if (sym.type() == Symbol::Nonlocal) {
                    // find symbol in parent (which is m_function)
                    for (const auto& psym : m_function.symtab()) {
                        if (psym.name() == sym.name()) {
                            // found it
                            switch (psym.type()) {
                                case Symbol::Nonlocal: {
                                    // COPY_ARGUMENT <frame_offset> <size>
                                    auto ofs_ti = m_function.nonlocal_offset_and_type(psym.index());
                                    assert(ofs_ti.first < 256);
                                    m_function.code().add_opcode(Opcode::CopyArgument,
                                        ofs_ti.first, ofs_ti.second.size());
                                    ofs_ti.second.foreach_heap_slot([this](size_t offset) {
                                        m_function.code().add_opcode(Opcode::IncRef, offset);
                                    });
                                    break;
                                }
                                case Symbol::Parameter: {
                                    // COPY_ARGUMENT <frame_offset> <size>
                                    const auto& ti = m_function.get_parameter(psym.index());
                                    m_function.code().add_opcode(Opcode::CopyArgument,
                                        m_function.parameter_offset(psym.index()) + nonlocals_size,
                                        ti.size());
                                    ti.foreach_heap_slot([this](size_t offset) {
                                        m_function.code().add_opcode(Opcode::IncRef, offset);
                                    });
                                    break;
                                }
                                case Symbol::Value: {
                                    // COPY_VARIABLE <frame_offset> <size>
                                    const auto& ti = m_function.get_value(psym.index());
                                    m_function.code().add_opcode(Opcode::CopyVariable,
                                        m_function.value_offset(psym.index()),
                                        ti.size());
                                    ti.foreach_heap_slot([this](size_t offset) {
                                        m_function.code().add_opcode(Opcode::IncRef, offset);
                                    });
                                    break;
                                }
                                default:
                                    break;
                            }
                            break;
                        }
                    }
                }
            }
            // MAKE_CLOSURE <function_idx> <n_nonlocals>
            m_function.code().add_opcode(Opcode::MakeClosure, v.index);
        } else {
            // LOAD_FUNCTION <function_idx>
            m_function.code().add_opcode(Opcode::LoadFunction, v.index);
        }
    }

    void visit(ast::TypeName& t) final {}
    void visit(ast::FunctionType& t) final {}

private:
    Module& module() { return m_function.module(); }

protected:
    Compiler& m_compiler;
    Function& m_function;
};


Compiler::Compiler()
{
    // module being compiled
    m_modules.push_back(make_unique<Module>());
    // module with builtins
    m_modules.push_back(make_unique<BuiltinModule>());
    module().add_imported_module(*m_modules.back());
}


void Compiler::configure(uint32_t flags)
{
    // each pass processes and modifies the AST - see documentation on each class
    m_ast_passes.clear();

    m_ast_passes.push_back(make_unique<SymbolResolver>());
    if ((flags & PPMask) == PPSymbols)
        return;

    m_ast_passes.push_back(make_unique<NonlocalResolver>());
    if ((flags & PPMask) == PPNonlocals)
        return;

    m_ast_passes.push_back(make_unique<TypeResolver>());
    if ((flags & PPMask) == PPTypes)
        return;

    if (flags & OConstFold) {
        m_ast_passes.push_back(make_unique<Optimizer>());
    }
}


void Compiler::compile(Function& func, AST& ast)
{
    func.signature().set_return_type(TypeInfo{Type::Auto});
    ast.body.symtab = &func.symtab();

    // Preprocess AST
    // - resolve symbols (SymbolResolver)
    // - infer and check types (TypeResolver)
    // - apply optimizations - const fold etc. (Optimizer)
    for (auto& proc : m_ast_passes) {
        proc->process_block(func, ast.body);
    }

    // Compile - only if mandatory passes were enabled
    if (m_ast_passes.size() >= 3)
        compile_block(func, ast.body);
}


void Compiler::compile_block(Function& func, const ast::Block& block)
{
    // Compile AST into bytecode
    CompilerVisitor visitor(*this, func);
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
}


} // namespace xci::script
