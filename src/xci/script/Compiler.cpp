// Compiler.cpp created on 2019-05-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Compiler.h"
#include "Error.h"
#include "ast/resolve_symbols.h"
#include "ast/resolve_decl.h"
#include "ast/resolve_types.h"
#include "ast/resolve_nonlocals.h"
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

    explicit CompilerVisitor(Compiler& compiler, FunctionScope& scope)
        : m_compiler(compiler), m_scope(scope) {}

    void visit(ast::Definition& dfn) override {
        if (!dfn.expression)
            return;  // it's only a declaration
        Function& fn = dfn.symbol().get_function(m_scope);

        if (fn.is_specialized())
            return;

        if (fn.has_any_generic()) {
            fn.ensure_ast_copy();
            return;
        }
        if (fn.is_undefined() || (fn.is_generic() && !fn.has_compile())) {
            fn.set_code();
            auto* orig_code = m_code;
            m_code = &fn.code();
            dfn.expression->apply(*this);
            m_code = orig_code;
            // compile_subroutine(func, *dfn.expression);*/
        }
    }

    void visit(ast::Invocation& inv) override {
        inv.expression->apply(*this);
        if (inv.type_id != no_index)
            code().add_L1(Opcode::Invoke, inv.type_id);
    }

    void visit(ast::Return& ret) override {
        ret.expression->apply(*this);

        if (function().has_intrinsics()) {
            if (function().intrinsics() != function().code().size())
                throw IntrinsicsFunctionError(
                        "cannot mix compiled code with intrinsics",
                        ret.expression->source_loc);
            // no DROP for intrinsics function
            return;
        }

        auto skip = function().signature().return_type.effective_type().size();
        auto drop = function().raw_size_of_parameters()
                  + function().raw_size_of_nonlocals()
                  + function().raw_size_of_partial();
        if (drop > 0) {
            Stack::StackRel pos = skip;
            for (const auto& ti : function().nonlocals()) {
                ti.foreach_heap_slot([this, pos](size_t offset) {
                    // DEC_REF <addr in nonlocals>
                    function().code().add_L1(Opcode::DecRef, pos + offset);
                });
                pos += ti.size();
            }
            for (const auto& ti : reverse(function().partial())) {
                ti.foreach_heap_slot([this, pos](size_t offset) {
                    // DEC_REF <addr in partial>
                    function().code().add_L1(Opcode::DecRef, pos + offset);
                });
                pos += ti.size();
            }
            for (const auto& ti : reverse(function().parameters())) {
                ti.foreach_heap_slot([this, pos](size_t offset) {
                    // DEC_REF <addr in params>
                    function().code().add_L1(Opcode::DecRef, pos + offset);
                });
                pos += ti.size();
            }
            // DROP <ret_value> <params + nonlocals>
            function().code().add_L2(Opcode::Drop, skip, drop);
        }
        // return value left on stack
    }

    void visit(ast::Literal& v) override {
        if (m_intrinsic) {
            m_instruction_args.push_back(v.value);
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

    void visit(ast::Tuple& v) override {
        // build tuple on stack
        if (v.literal_type.is_struct() && v.items.empty()) {
            // A struct can be initialized with empty tuple "()".
            // Fill in the defaults.
            for (const auto& ti : reverse(v.literal_type.struct_items())) {
                if (ti.second.is_void())
                    return;  // Void value
                // add to static values
                auto idx = module().add_value(TypedValue(ti.second));
                // LOAD_STATIC <static_idx>
                code().add_L1(Opcode::LoadStatic, idx);
            }
            return;
        }
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
        code().add_L2(Opcode::MakeList, v.items.size(), v.elem_type_id);
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
                    function().code().add_opcode(opcode);
                    function().add_intrinsics(1);
                } else if (opcode <= Opcode::B1ArgLast) {
                    assert(m_instruction_args.size() == 1);
                    auto arg = m_instruction_args[0].value().to_int64();
                    if (arg < 0 || arg >= 256)
                        throw IntrinsicsFunctionError("arg value out of Byte range: "
                                  + std::to_string(arg), v.source_loc);
                    function().code().add_B1(opcode, (uint8_t) arg);
                    function().add_intrinsics(2);
                } else if (opcode <= Opcode::L1ArgLast) {
                    assert(m_instruction_args.size() == 1);
                    auto arg = m_instruction_args[0].value().to_int64();
                    if (arg < 0)
                        throw IntrinsicsFunctionError("intrinsic argument is negative: "
                                                      + std::to_string(arg), v.source_loc);
                    auto n = function().code().add_L1(opcode, size_t(arg));
                    function().add_intrinsics(n);
                } else {
                    assert(opcode <= Opcode::L2ArgLast);
                    assert(m_instruction_args.size() == 2);
                    auto arg1 = m_instruction_args[0].value().to_int64();
                    if (arg1 < 0)
                        throw IntrinsicsFunctionError("intrinsic argument #1 is negative: "
                                                      + std::to_string(arg1), v.source_loc);
                    auto arg2 = m_instruction_args[1].value().to_int64();
                    if (arg2 < 0)
                        throw IntrinsicsFunctionError("intrinsic argument #2 is negative: "
                                                      + std::to_string(arg2), v.source_loc);
                    auto n = function().code().add_L2(opcode, size_t(arg1), size_t(arg2));
                    function().add_intrinsics(n);
                }
                break;
            }
            case Symbol::TypeId: {
                value::Int32 type_id_val((int32_t) v.index);
                if (m_intrinsic) {
                    m_instruction_args.emplace_back(type_id_val);
                    return;
                }

                // create static Int value in this module
                auto idx = module().add_value(TypedValue(type_id_val));
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

                if (is_nl_function && !function().partial().empty())
                    break;

                // Non-locals are captured in closure - read from closure
                auto ofs = m_scope.nonlocal_raw_offset(sym.index(), v.type_info);
                // COPY <frame_offset>
                code().add_L2(Opcode::Copy,
                              ofs, v.type_info.size());
                v.type_info.foreach_heap_slot([this](size_t offset) {
                    // INC_REF <stack_offset>
                    code().add_L1(Opcode::IncRef, offset);
                });
                break;
            }
            case Symbol::Value: {
                auto idx = sym.index();
                if (idx == no_index) {
                    // __value intrinsic
                    assert(m_intrinsic);
                    assert(m_instruction_args.size() == 1);
                    auto value_idx = module().add_value(TypedValue(m_instruction_args[0]));
                    m_instruction_args.clear();
                    m_instruction_args.emplace_back(value::Int64(value_idx));
                    break;
                }
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
                assert(sym.depth() == 0 && &symtab == &function().symtab());
                // COPY <frame_offset> <size>
                auto closure_size = function().raw_size_of_closure();
                const auto& ti = function().parameter(sym.index());
                code().add_L2(Opcode::Copy,
                        function().parameter_offset(sym.index()) + closure_size,
                        ti.size());
                ti.foreach_heap_slot([this](size_t offset) {
                    code().add_L1(Opcode::IncRef, offset);
                });
                break;
            }
            case Symbol::Method: {
                assert(v.index != no_index);
                FunctionScope& scope = v.module->get_scope(v.index);
                // this module
                if (v.module == &module()) {
                    // specialization might not be compiled yet - compile it now
                    Function& fn = scope.function();
                    if (fn.is_generic()) {
                        assert(!fn.has_any_generic());  // fully specialized
                        const auto body = fn.yank_generic_body();
                        fn.set_code();  // this would release AST copy
                        m_compiler.compile_function(scope, body.ast());
                    }

                    // CALL0 <function_idx>
                    code().add_L1(Opcode::Call0, scope.function_index());
                    break;
                }
                // builtin module or imported module
                auto mod_idx = module().get_imported_module_index(v.module);
                assert(mod_idx != no_index);
                if (mod_idx == 0) {
                    // CALL1 <function_idx>
                    code().add_L1(Opcode::Call1, scope.function_index());
                } else {
                    // CALL <module_idx> <function_idx>
                    code().add_L2(Opcode::Call, mod_idx, scope.function_index());
                }
                break;
            }
            case Symbol::Function: {
                assert(v.index != no_index);
                FunctionScope& scope = v.module->get_scope(v.index);
                // this module
                if (v.module == &module()) {
                    // specialization might not be compiled yet - compile it now
                    Function& fn = scope.function();
                    if (fn.is_generic()) {
                        assert(!fn.has_any_generic());  // fully specialized
                        const auto body = fn.yank_generic_body();
                        fn.set_code();  // this would release AST copy
                        m_compiler.compile_function(scope, body.ast());
                    }

                    if (scope.has_nonlocals()) {
                        make_closure(scope);
                        // MAKE_CLOSURE <function_idx>
                        code().add_L1(Opcode::MakeClosure, scope.function_index());
                        // EXECUTE
                        code().add_opcode(Opcode::Execute);
                    } else {
                        if (!m_callable && fn.has_parameters()) {
                            // LOAD_FUNCTION <function_idx>
                            code().add_L1(Opcode::LoadFunction, scope.function_index());
                        } else {
                            // CALL0 <function_idx>
                            code().add_L1(Opcode::Call0, scope.function_index());
                        }
                    }
                    break;
                }
                // builtin module or imported module
                auto mod_idx = module().get_imported_module_index(v.module);
                assert(mod_idx != no_index);
                if (mod_idx == 0) {
                    // CALL1 <function_idx>
                    code().add_L1(Opcode::Call1, scope.function_index());
                } else {
                    // CALL <module_idx> <function_idx>
                    code().add_L2(Opcode::Call, mod_idx, scope.function_index());
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
            case Symbol::StructItem: {
                // arg = struct (pushed on stack)
                const TypeInfo& struct_type = symtab.module()->get_type(sym.index());

                // return the item -> drop all other items from stack, leaving only the selected one
                size_t drop_before = 0;  // first DROP 0 <size>
                size_t skip = 0;
                size_t drop = 0;  // second DROP <skip> <size>
                for (const auto& item : struct_type.struct_items()) {
                    if (item.first == sym.name()) {
                        std::swap(drop_before, drop);
                        skip = item.second.size();
                        continue;
                    }
                    item.second.foreach_heap_slot([this, skip](size_t offset) {
                        // DEC_REF <offset>
                        function().code().add_L1(Opcode::DecRef, offset + skip);
                    });
                    drop += item.second.size();
                }
                if (drop_before != 0) {
                    // DROP <skip> <size>
                    function().code().add_L2(Opcode::Drop, 0, drop_before);
                }
                if (drop != 0) {
                    // DROP <skip> <size>
                    function().code().add_L2(Opcode::Drop, skip, drop);
                }
                break;
            }
            case Symbol::Unresolved:
                UNREACHABLE;
        }
        // if it's a function object, execute it
        if (sym.type() == Symbol::Nonlocal && sym.is_callable()) {
            code().add_opcode(Opcode::Execute);
        }
    }

    void visit(ast::Call& v) override {
        // call the function or push the function as value

        bool orig_callable = m_callable;
        m_intrinsic = v.intrinsic;
        m_instruction_args.clear();

        m_callable = false;
        for (auto& arg : reverse(v.args)) {
            arg->apply(*this);
        }
        m_callable = true;

        if (v.partial_index != no_index) {
            // partial function call
            auto& scope = module().get_scope(v.partial_index);
            auto& fn = scope.function();

            // generate inner code
            if (!fn.has_code())
                fn.set_code();

            compile_subroutine(scope, *v.callable);
            for (const auto& nl : fn.nonlocals()) {
                if (nl.is_callable() && nl.signature().has_closure()) {
                    // EXECUTE
                    fn.code().add_opcode(Opcode::Execute);
                }
            }

            make_closure(scope);
            if (!v.definition) {
                // MAKE_CLOSURE <function_idx>
                code().add_L1(Opcode::MakeClosure, scope.function_index());
                if (!fn.has_parameters()) {
                    // EXECUTE
                    code().add_opcode(Opcode::Execute);
                }
            }
        } else {
            v.callable->apply(*this);
        }

        // add executes for each call that results in function which consumes more args
        if (v.wrapped_execs > 1) {
            // Emit only a single EXECUTE if there is no closure in the wrapped function calls
            const Signature* sig = &v.callable_type.signature();
            for (auto i = v.wrapped_execs; i != 0; --i)
                sig = &sig->return_type.signature();
            if (!sig->has_closure())
                v.wrapped_execs = 1;
        }
        for (auto i = v.wrapped_execs; i != 0; --i)
            code().add_opcode(Opcode::Execute);

        m_intrinsic = false;
        m_callable = orig_callable;
    }

    void visit(ast::OpCall& v) override {
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        // See "Conditional jumps" in `docs/script/machine.adoc`
        std::vector<Code::OpIdx> end_arg_pos;
        for (auto& item : v.if_then_expr) {
            // condition
            item.first->apply(*this);
            // JUMP_IF_NOT (to next condition)
            code().add_B1(Opcode::JumpIfNot, 0);
            auto jump_arg_pos = code().this_instruction_address();
            // then branch
            item.second->apply(*this);
            // JUMP (to end)
            code().add_B1(Opcode::Jump, 0);
            end_arg_pos.push_back( code().this_instruction_address() );
            // .condX label
            // fill the above jump instruction target (jump here)
            const auto label_cond = code().this_instruction_address();
            code().set_arg_B(jump_arg_pos, label_cond - jump_arg_pos);
        }
        // else branch
        v.else_expr->apply(*this);
        // .end label
        // fill the target in all previous jump instructions
        const auto label_end = code().this_instruction_address();
        for (auto arg_pos : end_arg_pos) {
            code().set_arg_B(arg_pos, label_end - arg_pos);
        }
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
        FunctionScope& scope = module().get_scope(v.scope_index);
        Function& func = scope.function();
        if (func.has_any_generic()) {
            if (func.is_generic()) {
                func.ensure_ast_copy();  // generic function -> compiled on call
            } else {
                // has generic nonlocals, was specialized in resolve_nonlocals
                assert(func.signature().has_generic_nonlocals());
                func.set_undefined();
            }
            return;
        }

        if (!func.has_code()) {
            func.set_code();
            m_compiler.compile_function(scope, v.body);
        }

        if (func.symtab().parent() != &function().symtab())
            return;  // instance function -> just compile it
        if (scope.has_nonlocals()) {
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
                make_closure(scope);
                // MAKE_CLOSURE <function_idx>
                code().add_L1(Opcode::MakeClosure, scope.function_index());
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
                code().add_L1(Opcode::Call0, scope.function_index());
            } else {
                // push closure on stack (it may be EXECUTEd later by outer Call)
                make_closure(scope);
                // MAKE_CLOSURE <function_idx>
                code().add_L1(Opcode::MakeClosure, scope.function_index());
            }
        }
    }

    void visit(ast::Cast& v) override {
        v.expression->apply(*this);
        if (v.to_type.is_void()) {
            // cast to Void - remove the expression result from stack
            v.from_type.foreach_heap_slot([this](size_t offset) {
                // DEC_REF <offset>
                function().code().add_L1(Opcode::DecRef, offset);
            });
            // DROP 0 <size>
            function().code().add_L2(Opcode::Drop, 0, v.from_type.size());
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
    Module& module() { return m_scope.module(); }
    Function& function() { return m_scope.function(); }
    Code& code() { return m_code == nullptr ? function().code() : *m_code; }

    void make_closure(const FunctionScope& scope) {
        if (!scope.has_nonlocals())
            return;
        const auto& func = scope.function();
        // parent function
        assert(scope.parent() != nullptr);
        auto& parent_scope = *scope.parent();
        auto& parent_fn = parent_scope.function();
        auto closure_size = parent_fn.raw_size_of_closure();
        // make closure
        unsigned nl_i = scope.nonlocals().size();
        for (const auto& nl : reverse(scope.nonlocals())) {
            --nl_i;
            const auto& sym = *func.symtab().find_by_index(Symbol::Nonlocal, nl.index);
            assert(sym.depth() == 1 && sym.ref().symtab() == &parent_fn.symtab());
            const auto psym = *sym.ref();
            assert(psym.name() == sym.name());

            switch (psym.type()) {
                case Symbol::Nonlocal: {
                    // COPY <frame_offset> <size>
                    const auto& nl_ti = func.nonlocals()[nl_i];
                    auto ofs = parent_scope.nonlocal_raw_offset(psym.index(), nl_ti);
                    assert(ofs < 256);
                    code().add_L2(Opcode::Copy, ofs, nl_ti.size());
                    nl_ti.foreach_heap_slot([this](size_t offset) {
                        code().add_L1(Opcode::IncRef, offset);
                    });
                    break;
                }
                case Symbol::Parameter: {
                    // COPY <frame_offset> <size>
                    const auto& ti = func.nonlocals()[nl_i];
                    assert(ti == parent_fn.parameter(psym.index()));
                    code().add_L2(Opcode::Copy,
                            parent_fn.parameter_offset(psym.index()) + closure_size,
                            ti.size());
                    ti.foreach_heap_slot([this](size_t offset) {
                        code().add_L1(Opcode::IncRef, offset);
                    });
                    break;
                }
                case Symbol::Function: {
                    assert(nl.fn_scope_idx != no_index);
                    const auto& subscope = module().get_scope(nl.fn_scope_idx);
                    // const auto& subscope = parent_scope.get_subscope(psym.index());
                    Index fn_idx = subscope.function_index();
                    Function& fn = module().get_function(fn_idx);
                    if (subscope.has_nonlocals()) {
                        make_closure(subscope);
                        // MAKE_CLOSURE <function_idx>
                        code().add_L1(Opcode::MakeClosure, fn_idx);
                    } else if (fn.has_parameters()) {
                        // LOAD_FUNCTION <function_idx>
                        code().add_L1(Opcode::LoadFunction, fn_idx);
                    } else {
                        // CALL0 <function_idx>
                        code().add_L1(Opcode::Call0, fn_idx);
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    void compile_subroutine(FunctionScope& scope, ast::Expression& expression) {
        CompilerVisitor visitor(m_compiler, scope);
        expression.apply(visitor);
    }

private:
    Compiler& m_compiler;
    FunctionScope& m_scope;
    Code* m_code = nullptr;

    bool m_callable = true;

    // intrinsics
    bool m_intrinsic = false;
    std::vector<TypedValue> m_instruction_args;
};


bool Compiler::compile(FunctionScope& scope, ast::Module& ast)
{
    auto& func = scope.function();
    func.set_code();
    func.set_compile();
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
        resolve_symbols(scope, ast.body);

    if ((flags & Flags::ResolveDecl) == Flags::ResolveDecl)
        resolve_decl(scope, ast.body);

    if ((flags & Flags::ResolveTypes) == Flags::ResolveTypes)
        resolve_types(scope, ast.body);

    if ((flags & Flags::ResolveNonlocals) == Flags::ResolveNonlocals)
        resolve_nonlocals(scope, ast.body);

    if (default_compile)
        compile_all_functions(scope);

    if ((flags & Flags::FoldConstExpr) == Flags::FoldConstExpr)
        fold_const_expr(func, ast.body);

    if (default_compile)
        compile_function(scope, ast.body);

    return default_compile;
}


void Compiler::compile_function(FunctionScope& scope, const ast::Block& body)
{
    // Compile AST into bytecode
    CompilerVisitor visitor(*this, scope);
    for (const auto& stmt : body.statements) {
        stmt->apply(visitor);
    }
}


void Compiler::compile_all_functions(FunctionScope& main)
{
    Module& module = main.module();
    for (unsigned i = module.num_scopes(); i != 0; --i) {
        FunctionScope& scope = module.get_scope(i - 1);
        if (&scope == &main || !scope.has_function())
            continue;
        Function& fn = scope.function();
        if (!fn.is_generic())
            continue; // already compiled
        if (!fn.has_compile()) {
            // keep generic, make sure AST is copied
            assert(fn.has_any_generic());
            fn.ensure_ast_copy();
            continue;
        }

        assert(!fn.has_any_generic());
        const auto body = fn.yank_generic_body();
        fn.set_code();  // this removes AST from the function
        compile_function(scope, body.ast());
    }
}


} // namespace xci::script
