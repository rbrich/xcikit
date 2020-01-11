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
#include "NonlocalResolver.h"
#include "Stack.h"
#include <xci/compat/macros.h>

#include <range/v3/view/reverse.hpp>

#include <sstream>
#include <cassert>

namespace xci::script {

using namespace std;


class CompilerVisitor: public ast::Visitor {
public:
    explicit CompilerVisitor(Compiler& compiler, Function& function)
        : m_compiler(compiler), m_function(function) {}

    void visit(ast::Definition& dfn) override {
        Function& func = module().get_function(dfn.symbol()->index());
        auto* orig_code = m_code;
        m_code = &func.code();
        dfn.expression->apply(*this);
        m_code = orig_code;
        //compile_subroutine(func, *dfn.expression);
    }

    void visit(ast::Invocation& inv) override {
        inv.expression->apply(*this);
        code().add_opcode(Opcode::Invoke, inv.type_index);
    }

    void visit(ast::Return& ret) override {
        ret.expression->apply(*this);

        if (m_function.has_intrinsics()) {
            if (m_function.intrinsics() != m_function.code().size())
                throw IntrinsicsFunctionError("cannot mix compiled code with intrinsics");
            // no DROP for intrinsics function
            return;
        }

        auto skip = m_function.signature().return_type.effective_type().size();
        auto drop = m_function.raw_size_of_parameters()
                  + m_function.raw_size_of_nonlocals()
                  + m_function.raw_size_of_partial();
        if (drop > 0) {
            Stack::StackRel pos = skip;
            for (const auto& ti : m_function.nonlocals()) {
                ti.foreach_heap_slot([this, pos](size_t offset) {
                    // DEC_REF <addr in nonlocals>
                    m_function.code().add_opcode(Opcode::DecRef, pos + offset);
                });
                pos += ti.size();
            }
            for (const auto& ti : ranges::views::reverse(m_function.partial())) {
                ti.foreach_heap_slot([this, pos](size_t offset) {
                    // DEC_REF <addr in partial>
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
            // DROP <ret_value> <params + nonlocals>
            m_function.code().add_opcode(Opcode::Drop, skip, drop);
        }
        // return value left on stack
    }

    void visit(ast::Integer& v) override {
        // add to static values
        auto idx = module().add_value(std::make_unique<value::Int32>(v.value));
        // LOAD_STATIC <static_idx>
        code().add_opcode(Opcode::LoadStatic, idx);
    }

    void visit(ast::Float& v) override {
        // add to static values
        auto idx = module().add_value(std::make_unique<value::Float32>(v.value));
        // LOAD_STATIC <static_idx>
        code().add_opcode(Opcode::LoadStatic, idx);
    }

    void visit(ast::String& v) override {
        // add to static values
        auto idx = module().add_value(std::make_unique<value::String>(v.value));
        // LOAD_STATIC <static_idx>
        code().add_opcode(Opcode::LoadStatic, idx);
    }

    void visit(ast::Tuple& v) override {
        // build tuple on stack
        for (auto& item : ranges::views::reverse(v.items)) {
            item->apply(*this);
        }
    }

    void visit(ast::List& v) override {
        // build list
        for (auto& item : ranges::views::reverse(v.items)) {
            item->apply(*this);
        }
        // MAKE_LIST <length> <elem_size>
        code().add_opcode(Opcode::MakeList, v.items.size(), v.item_size);
    }

    void visit(ast::Reference& v) override {
        assert(v.identifier.symbol);
        auto& symtab = *v.identifier.symbol.symtab();
        auto& sym = *v.identifier.symbol;
        switch (sym.type()) {
            case Symbol::Class:
                assert(!"Class cannot be called.");
                break;
            case Symbol::Instance:
                assert(!"Instance cannot be called.");
                break;
            case Symbol::Method: {
                // this module
                if (v.module == &module()) {
                    // CALL0 <function_idx>
                    code().add_opcode(Opcode::Call0, v.index);
                    break;
                }
                // builtin module or imported module
                auto mod_idx = module().get_imported_module_index(v.module);
                assert(mod_idx != no_index);
                if (mod_idx == 0) {
                    // CALL1 <function_idx>
                    code().add_opcode(Opcode::Call1, v.index);
                } else {
                    // CALL <module_idx> <function_idx>
                    code().add_opcode(Opcode::Call, mod_idx, v.index);
                }
                break;
            }
            case Symbol::Instruction:
                // intrinsics - just output the requested instruction
                assert(sym.index() < 256);
                m_function.add_intrinsic(uint8_t(sym.index()));
                break;
            case Symbol::Module: {
                assert(sym.depth() == 0);
                // LOAD_MODULE <module_idx>
                code().add_opcode(Opcode::LoadModule, sym.index());
                break;
            }
            case Symbol::Nonlocal: {
                if (!m_function.partial().empty() && sym.ref()->type() == Symbol::Fragment) {
                    break;
                }

                // Non-locals are captured in closure - read from closure
                auto ofs_ti = m_function.nonlocal_offset_and_type(sym.index());
                // COPY <frame_offset>
                code().add_opcode(Opcode::Copy,
                        ofs_ti.first, ofs_ti.second.size());
                ofs_ti.second.foreach_heap_slot([this](size_t offset) {
                    // INC_REF <stack_offset>
                    code().add_opcode(Opcode::IncRef, offset);
                });
                break;
            }
            case Symbol::Value: {
                auto idx = sym.index();
                if (symtab.module() != &module()) {
                    // copy static value into this module if it's from builtin or another module
                    auto& val = symtab.module()->get_value(sym.index());
                    idx = module().add_value(val.make_copy());
                }
                // LOAD_STATIC <static_idx>
                code().add_opcode(Opcode::LoadStatic, idx);
                break;
            }
            case Symbol::Parameter: {
                assert(sym.depth() == 0);
                // COPY <frame_offset> <size>
                auto closure_size = m_function.raw_size_of_closure();
                const auto& ti = m_function.get_parameter(sym.index());
                code().add_opcode(Opcode::Copy,
                        m_function.parameter_offset(sym.index()) + closure_size,
                        ti.size());
                ti.foreach_heap_slot([this](size_t offset) {
                    code().add_opcode(Opcode::IncRef, offset);
                });
                break;
            }
            case Symbol::Function: {
                // this module
                if (symtab.module() == nullptr || symtab.module() == &module()) {
                    // specialization might not be compiled yet - compile it now
                    Function& func = module().get_function(sym.index());
                    if (func.has_ast()) {
                        m_compiler.compile_block(func, *func.ast());
                        func.set_ast(nullptr);
                    }
                    // CALL0 <function_idx>
                    code().add_opcode(Opcode::Call0, sym.index());
                    break;
                }
                // builtin module or imported module
                auto mod_idx = module().get_imported_module_index(symtab.module());
                assert(mod_idx != no_index);
                if (mod_idx == 0) {
                    // CALL1 <function_idx>
                    code().add_opcode(Opcode::Call1, sym.index());
                } else {
                    // CALL <module_idx> <function_idx>
                    code().add_opcode(Opcode::Call, mod_idx, sym.index());
                }
                break;
            }
            case Symbol::Fragment: {
                assert(symtab.module() == nullptr || symtab.module() == &module());
                Function& func = module().get_function(sym.index());
                // inline the code
                int levels = 0;
                auto* scope = &m_function.symtab();
                while (scope != func.symtab().parent()) {
                    scope = scope->parent();
                    ++levels;
                }
                if (levels != 0) {
                    // SET_BASE <levels>
                    code().add_opcode(Opcode::SetBase, levels);
                }
                // copy the code
                for (auto instr : func.code()) {
                    code().add(instr);
                }
                break;
            }
            case Symbol::TypeName:
            case Symbol::TypeVar:
                // TODO
                break;
            case Symbol::Unresolved:
                UNREACHABLE;
        }
        // if it's function object, execute it
        if (sym.type() != Symbol::Function
        &&  sym.type() != Symbol::Method
        &&  sym.is_callable()) {
            code().add_opcode(Opcode::Execute);
        }
    }

    void visit(ast::Call& v) override {
        // call the function or push the value

        for (auto& arg : ranges::views::reverse(v.args)) {
            arg->apply(*this);
        }

        if (v.partial_index != no_index) {
            // partial function call
            auto& fn = module().get_function(v.partial_index);

            // generate inner code
            compile_subroutine(fn, *v.callable);
            for (const auto& nl : fn.nonlocals()) {
                if (nl.is_callable() && nl.signature().has_closure()) {
                    // EXECUTE
                    fn.code().add_opcode(Opcode::Execute);
                }
            }

            make_closure(fn);
            if (!v.definition) {
                // MAKE_CLOSURE <function_idx>
                code().add_opcode(Opcode::MakeClosure, v.partial_index);
                if (!fn.has_parameters()) {
                    // EXECUTE
                    code().add_opcode(Opcode::Execute);
                }
            }
        } else {
            v.callable->apply(*this);
        }

        // add executes for each call that results in function which consumes more args
        while (v.wrapped_execs > 0) {
            code().add_opcode(Opcode::Execute);
            -- v.wrapped_execs;
        }
    }

    void visit(ast::OpCall& v) override {
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        // evaluate condition
        v.cond->apply(*this);
        // add jump instruction
        code().add_opcode(Opcode::JumpIfNot, 0);
        auto jump1_arg_pos = code().this_instruction_address();
        // then branch
        v.then_expr->apply(*this);
        code().add_opcode(Opcode::Jump, 0);
        auto jump2_arg_pos = code().this_instruction_address();
        // else branch
        code().set_arg(jump1_arg_pos,
                code().this_instruction_address() - jump1_arg_pos);
        v.else_expr->apply(*this);
        // end
        code().set_arg(jump2_arg_pos,
                code().this_instruction_address() - jump2_arg_pos);
    }

    void visit(ast::Function& v) override {
        // compile body
        Function& func = module().get_function(v.index);
        if (func.has_ast())
            return;  // generic function -> compiled on call

        m_compiler.compile_block(func, v.body);
        if (func.symtab().parent() != &m_function.symtab())
            return;  // instance function -> just compile it
        if (func.has_nonlocals()) {
            if (v.definition) {
                // fill wrapping function for the closure
                auto& wfn = module().get_function(v.definition->symbol()->index());
                auto* orig_code = m_code;
                m_code = &wfn.code();
                make_closure(func);
                // MAKE_CLOSURE <function_idx>
                code().add_opcode(Opcode::MakeClosure, v.index);
                if (!func.has_parameters()) {
                    // parameterless closure is executed immediately
                    // EXECUTE
                    code().add_opcode(Opcode::Execute);
                }
                m_code = orig_code;
            } else {
                make_closure(func);
                // MAKE_CLOSURE <function_idx>
                code().add_opcode(Opcode::MakeClosure, v.index);
                if (!func.has_parameters()) {
                    // parameterless closure is executed immediately
                    // EXECUTE
                    code().add_opcode(Opcode::Execute);
                }
            }
        } else {
            if (!v.definition) {
                // CALL0 <function_idx>
                code().add_opcode(Opcode::Call0, v.index);
            }
        }
    }

    void visit(ast::Class& v) override {
        // TODO
    }

    void visit(ast::Instance& v) override {
        for (auto& dfn : v.defs)
            dfn.apply(*this);
    }

    void visit(ast::TypeName& t) final {}
    void visit(ast::FunctionType& t) final {}
    void visit(ast::ListType& t) final {}

private:
    Module& module() { return m_function.module(); }
    Code& code() { return m_code == nullptr ? m_function.code() : *m_code; };

    void make_closure(Function& func) {
        // m_function is parent
        assert(&m_function.symtab() == func.symtab().parent());
        auto closure_size = m_function.raw_size_of_closure();
        // make closure
        for (const auto& sym : ranges::views::reverse(func.symtab())) {
            if (sym.type() == Symbol::Nonlocal) {
                // find symbol in parent (which is m_function)
                for (const auto& psym : m_function.symtab()) {
                    if (psym.name() == sym.name()) {
                        // found it
                        switch (psym.type()) {
                            case Symbol::Nonlocal: {
                                // COPY <frame_offset> <size>
                                auto ofs_ti = m_function.nonlocal_offset_and_type(psym.index());
                                assert(ofs_ti.first < 256);
                                code().add_opcode(Opcode::Copy,
                                        ofs_ti.first, ofs_ti.second.size());
                                ofs_ti.second.foreach_heap_slot([this](size_t offset) {
                                    code().add_opcode(Opcode::IncRef, offset);
                                });
                                break;
                            }
                            case Symbol::Parameter: {
                                // COPY <frame_offset> <size>
                                const auto& ti = m_function.get_parameter(psym.index());
                                code().add_opcode(Opcode::Copy,
                                        m_function.parameter_offset(psym.index()) + closure_size,
                                        ti.size());
                                ti.foreach_heap_slot([this](size_t offset) {
                                    code().add_opcode(Opcode::IncRef, offset);
                                });
                                break;
                            }
                            case Symbol::Fragment: {
                                Function& fragment = module().get_function(psym.index());
                                // inline the code
                                int levels = 0;
                                auto* scope = &m_function.symtab();
                                while (scope != fragment.symtab().parent()) {
                                    scope = scope->parent();
                                    ++levels;
                                }
                                if (levels != 0) {
                                    // SET_BASE <levels>
                                    code().add_opcode(Opcode::SetBase, levels);
                                }
                                // copy the code
                                for (auto instr : fragment.code()) {
                                    code().add(instr);
                                }
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
    }

    void compile_subroutine(Function& func, ast::Expression& expression) {
        CompilerVisitor visitor(m_compiler, func);
        expression.apply(visitor);
    }

private:
    Compiler& m_compiler;
    Function& m_function;
    Code* m_code = nullptr;
};


void Compiler::configure(uint32_t flags)
{
    // each pass processes and modifies the AST - see documentation on each class
    m_ast_passes.clear();
    m_compile = false;

    m_ast_passes.push_back(make_unique<SymbolResolver>());
    if ((flags & PPMask) == PPSymbols)
        return;

    m_ast_passes.push_back(make_unique<TypeResolver>());
    if ((flags & PPMask) == PPTypes)
        return;

    m_ast_passes.push_back(make_unique<NonlocalResolver>());
    if ((flags & PPMask) == PPNonlocals)
        return;

    if (flags & OConstFold) {
        // FIXME: update Optimizer
        //m_ast_passes.push_back(make_unique<Optimizer>());
    }

    m_compile = true;
}


void Compiler::compile(Function& func, ast::Module& ast)
{
    func.signature().set_return_type(TypeInfo{Type::Unknown});
    ast.body.symtab = &func.symtab();

    // Preprocess AST
    // - resolve symbols (SymbolResolver)
    // - infer and check types (TypeResolver)
    // - apply optimizations - const fold etc. (Optimizer)
    for (auto& proc : m_ast_passes) {
        proc->process_block(func, ast.body);
    }

    // Compile - only if mandatory passes were enabled
    if (m_compile)
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
