// Compiler.cpp created on 2019-05-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Compiler.h"
#include "Error.h"
#include "ast/resolve_symbols.h"
#include "ast/resolve_nonlocals.h"
#include "ast/resolve_types.h"
#include "ast/fold_const_expr.h"
#include "ast/fold_dot_call.h"
#include "ast/fold_tuple.h"
#include "Stack.h"
#include <xci/compat/macros.h>

#include <range/v3/view/reverse.hpp>

#include <sstream>
#include <cassert>

namespace xci::script {

using ranges::cpp20::views::reverse;


class CompilerVisitor: public ast::VisitorExclTypes {
public:
    using VisitorExclTypes::visit;

    explicit CompilerVisitor(Compiler& compiler, Function& function)
        : m_compiler(compiler), m_function(function) {}

    void visit(ast::Definition& dfn) override {
        Function& func = module().get_function(dfn.symbol()->index());
        if (func.detect_generic()) {
            func.ensure_ast_copy();
            return;
        }
        if (func.is_undefined())
            func.set_compiled();
        assert(func.is_compiled());
        auto* orig_code = m_code;
        m_code = &func.code();
        dfn.expression->apply(*this);
        m_code = orig_code;
        //compile_subroutine(func, *dfn.expression);
    }

    void visit(ast::Invocation& inv) override {
        inv.expression->apply(*this);
        if (inv.type_id != no_index)
            code().add_L1(Opcode::Invoke, inv.type_id);
    }

    void visit(ast::Return& ret) override {
        ret.expression->apply(*this);

        if (m_function.has_intrinsics()) {
            if (m_function.intrinsics() != m_function.code().size())
                throw IntrinsicsFunctionError(
                        "cannot mix compiled code with intrinsics",
                        ret.expression->source_loc);
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
                    m_function.code().add_L1(Opcode::DecRef, pos + offset);
                });
                pos += ti.size();
            }
            for (const auto& ti : reverse(m_function.partial())) {
                ti.foreach_heap_slot([this, pos](size_t offset) {
                    // DEC_REF <addr in partial>
                    m_function.code().add_L1(Opcode::DecRef, pos + offset);
                });
                pos += ti.size();
            }
            for (const auto& ti : reverse(m_function.parameters())) {
                ti.foreach_heap_slot([this, pos](size_t offset) {
                    // DEC_REF <addr in params>
                    m_function.code().add_L1(Opcode::DecRef, pos + offset);
                });
                pos += ti.size();
            }
            // DROP <ret_value> <params + nonlocals>
            m_function.code().add_L2(Opcode::Drop, skip, drop);
        }
        // return value left on stack
    }

    void visit(ast::Literal& v) override {
        if (m_intrinsic) {
            switch (v.value.type()) {
                case Type::Byte:
                    m_instruction_args.push_back((uint32_t) v.value.get<byte>());
                    break;
                case Type::Int32:
                    m_instruction_args.push_back(v.value.get<int32_t>());
                    break;
                default:
                    assert(!"wrong intrinsic argument type");
                    break;
            }
            return;
        }

        if (v.value.is_void())
            return;  // Void value
        // add to static values
        v.value.incref();
        auto idx = module().add_value(TypedValue(v.value));
        // LOAD_STATIC <static_idx>
        code().add_L1(Opcode::LoadStatic, idx);
    }

    void visit(ast::Bracketed& v) override {
        v.expression->apply(*this);
    }

    void visit(ast::Tuple& v) override {
        // build tuple on stack
        for (auto& item : reverse(v.items)) {
            item->apply(*this);
        }
    }

    void visit(ast::List& v) override {
        // build list
        for (auto& item : reverse(v.items)) {
            item->apply(*this);
        }
        // MAKE_LIST <length> <elem_type>
        code().add_L2(Opcode::MakeList, v.items.size(), v.elem_type_idx);
    }

    void visit(ast::StructInit& v) override {
        // build struct on stack according to struct_type, fill in defaults
        for (const auto& ti : reverse(v.struct_type.struct_items())) {
            // lookup the name in StructInit
            auto it = std::find_if(v.items.begin(), v.items.end(),
                [&ti](const ast::StructInit::Item& item) {
                  return item.first.name == ti.first;
                });
            if (it == v.items.end()) {
                // not found - use the default
                if (ti.second.is_void())
                    return;  // Void value
                // add to static values
                auto idx = module().add_value(TypedValue(ti.second));
                // LOAD_STATIC <static_idx>
                code().add_L1(Opcode::LoadStatic, idx);
                continue;
            }
            it->second->apply(*this);
        }
    }

    void visit(ast::Reference& v) override {
        assert(v.identifier.symbol);
        auto& symtab = *v.identifier.symbol.symtab();
        const auto& sym = *v.identifier.symbol;
        switch (sym.type()) {
            case Symbol::Instruction: {
                // intrinsics - just output the requested instruction
                auto opcode = Opcode(sym.index());
                if (opcode <= Opcode::NoArgLast) {
                    m_function.code().add_opcode(opcode);
                    m_function.add_intrinsics(1);
                } else if (opcode <= Opcode::B1ArgLast) {
                    assert(m_instruction_args.size() == 1);
                    if (m_instruction_args[0] >= 256)
                        throw IntrinsicsFunctionError("arg value out of Byte range: "
                                  + std::to_string(m_instruction_args[0]), v.source_loc);
                    m_function.code().add_B1(opcode, (uint8_t) m_instruction_args[0]);
                    m_function.add_intrinsics(2);
                } else if (opcode <= Opcode::L1ArgLast) {
                    assert(m_instruction_args.size() == 1);
                    auto n = m_function.code().add_L1(opcode, m_instruction_args[0]);
                    m_function.add_intrinsics(n);
                } else {
                    assert(opcode <= Opcode::L2ArgLast);
                    assert(m_instruction_args.size() == 2);
                    auto n = m_function.code().add_L2(opcode,
                            m_instruction_args[0], m_instruction_args[1]);
                    m_function.add_intrinsics(n);
                }
                break;
            }
            case Symbol::TypeId: {
                auto type_id = sym.index();
                if (m_intrinsic) {
                    m_instruction_args.push_back(type_id);
                    return;
                }

                // create static Int value in this module
                auto idx = module().add_value(TypedValue(value::Int32((int32_t)type_id)));
                // LOAD_STATIC <static_idx>
                code().add_L1(Opcode::LoadStatic, idx);
                break;
            }
            case Symbol::Module: {
                assert(sym.depth() == 0);
                // LOAD_MODULE <module_idx>
                code().add_L1(Opcode::LoadModule, sym.index());
                break;
            }
            case Symbol::Nonlocal: {
                bool is_nl_function = (sym.ref()->type() == Symbol::Function);
                if (is_nl_function && v.module == &module()) {
                    // specialization might not be compiled yet - compile it now
                    Function& fn = module().get_function(v.index);
                    if (fn.is_generic()) {
                        assert(!fn.detect_generic());  // fully specialized
                        const auto body = fn.yank_generic_body();
                        fn.set_compiled();  // this would release AST copy
                        m_compiler.compile_block(fn, body.ast());
                    }
                }

                if (is_nl_function && !m_function.partial().empty())
                    break;

                // Non-locals are captured in closure - read from closure
                auto ofs_ti = m_function.nonlocal_offset_and_type(sym.index());
                // COPY <frame_offset>
                code().add_L2(Opcode::Copy,
                        ofs_ti.first, ofs_ti.second.size());
                ofs_ti.second.foreach_heap_slot([this](size_t offset) {
                    // INC_REF <stack_offset>
                    code().add_L1(Opcode::IncRef, offset);
                });
                break;
            }
            case Symbol::Value: {
                auto idx = sym.index();
                if (symtab.module() != &module()) {
                    // copy static value into this module if it's from builtin or another module
                    const auto & val = symtab.module()->get_value(sym.index());
                    val.incref();
                    idx = module().add_value(TypedValue(val));
                }
                // LOAD_STATIC <static_idx>
                code().add_L1(Opcode::LoadStatic, idx);
                break;
            }
            case Symbol::Parameter: {
                assert(sym.depth() == 0);
                // COPY <frame_offset> <size>
                auto closure_size = m_function.raw_size_of_closure();
                const auto& ti = m_function.parameter(sym.index());
                code().add_L2(Opcode::Copy,
                        m_function.parameter_offset(sym.index()) + closure_size,
                        ti.size());
                ti.foreach_heap_slot([this](size_t offset) {
                    code().add_L1(Opcode::IncRef, offset);
                });
                break;
            }
            case Symbol::Method: {
                // this module
                if (v.module == &module()) {
                    // CALL0 <function_idx>
                    code().add_L1(Opcode::Call0, v.index);
                    break;
                }
                // builtin module or imported module
                auto mod_idx = module().get_imported_module_index(v.module);
                assert(mod_idx != no_index);
                if (mod_idx == 0) {
                    // CALL1 <function_idx>
                    code().add_L1(Opcode::Call1, v.index);
                } else {
                    // CALL <module_idx> <function_idx>
                    code().add_L2(Opcode::Call, mod_idx, v.index);
                }
                break;
            }
            case Symbol::Function: {
                assert(v.index != no_index);
                // this module
                if (v.module == &module()) {
                    // specialization might not be compiled yet - compile it now
                    Function& fn = module().get_function(v.index);
                    if (fn.is_generic()) {
                        assert(!fn.detect_generic());  // fully specialized
                        const auto body = fn.yank_generic_body();
                        fn.set_compiled();  // this would release AST copy
                        m_compiler.compile_block(fn, body.ast());
                    }

                    if (fn.has_nonlocals()) {
                        make_closure(fn);
                        // MAKE_CLOSURE <function_idx>
                        code().add_L1(Opcode::MakeClosure, v.index);
                        // EXECUTE
                        code().add_opcode(Opcode::Execute);
                    } else {
                        // CALL0 <function_idx>
                        code().add_L1(Opcode::Call0, v.index);
                    }
                    break;
                }
                // builtin module or imported module
                auto mod_idx = module().get_imported_module_index(v.module);
                assert(mod_idx != no_index);
                if (mod_idx == 0) {
                    // CALL1 <function_idx>
                    code().add_L1(Opcode::Call1, v.index);
                } else {
                    // CALL <module_idx> <function_idx>
                    code().add_L2(Opcode::Call, mod_idx, v.index);
                }
                break;
            }
            case Symbol::Class:
                assert(!"Class cannot be called.");
                break;
            case Symbol::Instance:
                assert(!"Instance cannot be called.");
                break;
            case Symbol::TypeName:
            case Symbol::TypeVar:
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
        // call the function or push the function as value

        m_intrinsic = v.intrinsic;

        for (auto& arg : reverse(v.args)) {
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
                code().add_L1(Opcode::MakeClosure, v.partial_index);
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

        m_intrinsic = false;
    }

    void visit(ast::OpCall& v) override {
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        // evaluate condition
        v.cond->apply(*this);
        // add jump instruction
        code().add_B1(Opcode::JumpIfNot, 0);
        auto jump1_arg_pos = code().this_instruction_address();
        // then branch
        v.then_expr->apply(*this);
        code().add_B1(Opcode::Jump, 0);
        auto jump2_arg_pos = code().this_instruction_address();
        // else branch
        code().set_arg_B(jump1_arg_pos,
                code().this_instruction_address() - jump1_arg_pos);
        v.else_expr->apply(*this);
        // end
        code().set_arg_B(jump2_arg_pos,
                code().this_instruction_address() - jump2_arg_pos);
    }

    void visit(ast::WithContext& v) override {
        // evaluate context
        v.context->apply(*this);
        // call the enter function - pulls arg and pushes the context to stack
        v.enter_function.apply(*this);
        // inner expression
        v.expression->apply(*this);
        // swap the expression result with context data below it
        // SWAP <result_size> <context_size>
        code().add_L2(Opcode::Swap,
                v.expression_type.size(), v.leave_type.size());
        // call the leave function - must pull the context from enter function
        v.leave_function.apply(*this);
    }

    void visit(ast::Function& v) override {
        // compile body
        Function& func = module().get_function(v.index);
        if (func.detect_generic()) {
            func.ensure_ast_copy();
            return;  // generic function -> compiled on call
        }

        m_compiler.compile_block(func, v.body);
        if (func.symtab().parent() != &m_function.symtab())
            return;  // instance function -> just compile it
        if (func.has_nonlocals()) {
            if (v.definition) {
                /*if (!func.has_parameters()) {
                    // parameterless closure is executed immediately
                    make_closure(func);
                    // MAKE_CLOSURE <function_idx>
                    code().add_L1(Opcode::MakeClosure, v.index);
                    // EXECUTE
                    code().add_opcode(Opcode::Execute);
                }*/
            } else {
                make_closure(func);
                // MAKE_CLOSURE <function_idx>
                code().add_L1(Opcode::MakeClosure, v.index);
                if (!func.has_parameters()) {
                    // parameterless closure is executed immediately
                    // EXECUTE
                    code().add_opcode(Opcode::Execute);
                }
            }
        } else if (!v.definition) {
            // call the function only if it's inside a Call which applies all required parameters (might be zero)
            if (v.call_args >= func.parameters().size()) {
                // CALL0 <function_idx>
                code().add_L1(Opcode::Call0, v.index);
            } else {
                // push closure on stack (it may be EXECUTEd later by outer Call)
                make_closure(func);
                // MAKE_CLOSURE <function_idx>
                code().add_L1(Opcode::MakeClosure, v.index);
            }
        }
    }

    void visit(ast::Cast& v) override {
        v.expression->apply(*this);
        if (v.to_type.is_void()) {
            // cast to Void - remove the expression result from stack
            v.from_type.foreach_heap_slot([this](size_t offset) {
                // DEC_REF <offset>
                m_function.code().add_L1(Opcode::DecRef, offset);
            });
            // DROP 0 <size>
            m_function.code().add_L2(Opcode::Drop, 0, v.from_type.size());
            return;
        }
        if (v.cast_function)
            v.cast_function->apply(*this);
    }

    void visit(ast::Instance& v) override {
        for (auto& dfn : v.defs)
            dfn.apply(*this);
    }

    void visit(ast::Class& v) override {}

private:
    Module& module() { return m_function.module(); }
    Code& code() { return m_code == nullptr ? m_function.code() : *m_code; };

    void make_closure(Function& func) {
        // parent function
        assert(func.symtab().parent()->function() != nullptr);
        auto& parent = *func.symtab().parent()->function();
        auto closure_size = parent.raw_size_of_closure();
        // make closure
        for (const auto& sym : reverse(func.symtab())) {
            if (sym.type() == Symbol::Nonlocal) {
                //assert(sym.depth() == 1);
                // find symbol in parent
                for (const auto& psym : parent.symtab()) {
                    if (psym.name() == sym.name()) {
                        // found it
                        switch (psym.type()) {
                            case Symbol::Nonlocal: {
                                // COPY <frame_offset> <size>
                                auto ofs_ti = parent.nonlocal_offset_and_type(psym.index());
                                assert(ofs_ti.first < 256);
                                code().add_L2(Opcode::Copy,
                                        ofs_ti.first, ofs_ti.second.size());
                                ofs_ti.second.foreach_heap_slot([this](size_t offset) {
                                    code().add_L1(Opcode::IncRef, offset);
                                });
                                break;
                            }
                            case Symbol::Parameter: {
                                // COPY <frame_offset> <size>
                                const auto& ti = parent.parameter(psym.index());
                                code().add_L2(Opcode::Copy,
                                        parent.parameter_offset(psym.index()) + closure_size,
                                        ti.size());
                                ti.foreach_heap_slot([this](size_t offset) {
                                    code().add_L1(Opcode::IncRef, offset);
                                });
                                break;
                            }
                            case Symbol::Function: {
                                Function& fn = module().get_function(psym.index());
                                if (fn.has_nonlocals()) {
                                    make_closure(fn);
                                    // MAKE_CLOSURE <function_idx>
                                    code().add_L1(Opcode::MakeClosure, psym.index());
                                } else if (fn.has_parameters()) {
                                    // LOAD_FUNCTION <function_idx>
                                    code().add_L1(Opcode::LoadFunction, psym.index());
                                } else {
                                    // CALL0 <function_idx>
                                    code().add_L1(Opcode::Call0, psym.index());
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

    // intrinsics
    bool m_intrinsic = false;
    std::vector<size_t> m_instruction_args;
};


bool Compiler::compile(Function& func, ast::Module& ast)
{
    func.set_compiled();
    func.signature().set_return_type(ti_unknown());
    ast.body.symtab = &func.symtab();

    // Preprocess AST
    // - resolve symbols (SymbolResolver)
    // - infer and check types (TypeResolver)
    // - apply optimizations - const fold etc. (Optimizer)
    // See documentation on each function.

    // If these flags are not Default, don't compile, only preprocess
    auto flags = m_flags;
    bool default_compile = (flags & Flags::MandatoryMask) == Flags::Default;
    if (default_compile) {
        // Enable all mandatory passes for default compilation
        flags |= Flags::MandatoryMask;
    }

    if ((flags & Flags::FoldTuple) == Flags::FoldTuple)
        fold_tuple(ast.body);

    if ((flags & Flags::FoldDotCall) == Flags::FoldDotCall)
        fold_dot_call(func, ast.body);

    if ((flags & Flags::ResolveSymbols) == Flags::ResolveSymbols)
        resolve_symbols(func, ast.body);

    if ((flags & Flags::ResolveTypes) == Flags::ResolveTypes)
        resolve_types(func, ast.body);

    if ((flags & Flags::FoldConstExpr) == Flags::FoldConstExpr)
        fold_const_expr(func, ast.body);

    if ((flags & Flags::ResolveNonlocals) == Flags::ResolveNonlocals)
        resolve_nonlocals(func, ast.body);

    if (default_compile)
        compile_block(func, ast.body);
    return default_compile;
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
