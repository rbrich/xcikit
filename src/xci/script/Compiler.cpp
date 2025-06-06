// Compiler.cpp created on 2019-05-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Compiler.h"
#include "Error.h"
#include "ast/resolve_symbols.h"
#include "ast/resolve_decl.h"
#include "ast/resolve_types.h"
#include "ast/resolve_spec.h"
#include "ast/resolve_nonlocals.h"
#include "ast/fold_const_expr.h"
#include "ast/fold_dot_call.h"
#include "ast/fold_tuple.h"
#include "ast/fold_paren.h"
#include "code/optimize_tail_call.h"
#include "code/optimize_copy_drop.h"
#include "typing/type_index.h"
#include "Stack.h"
#include <xci/compat/macros.h>

#include <ranges>
#include <sstream>
#include <cassert>

namespace xci::script {

using std::ranges::views::reverse;


class CompilerVisitor: public ast::VisitorExclTypes {
public:
    using VisitorExclTypes::visit;

    explicit CompilerVisitor(Compiler& compiler, Scope& scope)
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
        assert(!fn.is_undefined());
    }

    void visit(ast::Invocation& inv) override {
        inv.expression->apply(*this);
        // Unknown in intrinsics function
        if (!inv.ti.is_void() && !inv.ti.is_unknown()) {
            const Index type_index = make_type_index(module(), inv.ti);
            code().add_L1(Opcode::Invoke, type_index);
        }
    }

    void visit(ast::Return& ret) override {
        ret.expression->apply(*this);
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
        if (v.ti.underlying().is_struct() && v.items.empty()) {
            // A struct can be initialized with empty tuple "()".
            // Fill in the defaults.
            for (const auto& ti : reverse(v.ti.underlying().subtypes())) {
                if (ti.is_void())
                    return;  // Void value
                // add to static values
                auto idx = module().add_value(TypedValue(ti));
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
        const Index type_index = make_type_index(module(), v.ti.underlying().elem_type());
        code().add_L2(Opcode::MakeList, v.items.size(), type_index);
    }

    void visit(ast::StructInit& v) override {
        // build struct on stack according to struct_type, fill in defaults
        for (const auto& ti : reverse(v.ti.underlying().subtypes())) {
            // lookup the name in StructInit
            const auto it = std::ranges::find_if(v.items,
                [&ti](const ast::StructInit::Item& item) {
                  return item.first.name == ti.key();
                });
            if (it == v.items.end()) {
                // not found - use the default
                if (ti.is_void())
                    return;  // Void value
                // add to static values
                auto idx = module().add_value(TypedValue(ti));
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
                function().add_intrinsics();
                auto opcode = Opcode(sym.index());
                if (opcode <= Opcode::A0Last) {
                    code().add(opcode);
                } else if (opcode <= Opcode::B1Last) {
                    assert(m_instruction_args.size() == 1);
                    auto arg = m_instruction_args[0].value().to_int64();
                    if (arg < 0 || arg >= 256)
                        throw intrinsics_function_error("arg value out of Byte range: "
                                                        + std::to_string(arg), v.source_loc);
                    code().add_B1(opcode, (uint8_t) arg);
                } else if (opcode <= Opcode::L1Last) {
                    assert(m_instruction_args.size() == 1);
                    auto arg = m_instruction_args[0].value().to_int64();
                    if (arg < 0)
                        throw intrinsics_function_error("intrinsic argument is negative: "
                                                        + std::to_string(arg), v.source_loc);
                    code().add_L1(opcode, size_t(arg));
                } else {
                    assert(opcode <= Opcode::L2Last);
                    assert(m_instruction_args.size() == 2);
                    auto arg1 = m_instruction_args[0].value().to_int64();
                    if (arg1 < 0)
                        throw intrinsics_function_error("intrinsic argument #1 is negative: "
                                                        + std::to_string(arg1), v.source_loc);
                    auto arg2 = m_instruction_args[1].value().to_int64();
                    if (arg2 < 0)
                        throw intrinsics_function_error("intrinsic argument #2 is negative: "
                                                        + std::to_string(arg2), v.source_loc);
                    code().add_L2(opcode, size_t(arg1), size_t(arg2));
                }
                break;
            }
            case Symbol::TypeIndex: {
                const Index index = make_type_index(module(), v.ti);
                const value::TypeIndex type_id_val((int32_t) index);
                if (m_intrinsic) {
                    m_instruction_args.emplace_back(type_id_val);
                    return;
                }

                // create static TypeIndex value in this module
                auto idx = module().add_value(TypedValue(type_id_val));
                // LOAD_STATIC <static_idx>
                code().add_L1(Opcode::LoadStatic, idx);
                break;
            }
            case Symbol::Module: {
                assert(sym.depth() == 0);
                if (sym.index() == no_index) {
                    // builtin __module
                    if (m_instruction_args.empty()) {
                        // LOAD_MODULE <module_idx>
                        code().add_L1(Opcode::LoadModule, no_index);
                    } else {
                        assert(m_intrinsic);
                        assert(m_instruction_args.size() == 1);
                        const Index module_idx = m_instruction_args[0].value().to_int64();
                        assert(module_idx < m_scope.module().num_imported_modules());
                        // LOAD_MODULE <module_idx>
                        code().add_L1(Opcode::LoadModule, module_idx);
                    }
                    break;
                }
                // Call main function of the module
                // CALL <module_idx> <function_idx>
                code().add_L2(Opcode::Call, sym.index(), 0);
                break;
            }
            case Symbol::Nonlocal: {
                // Non-locals are captured in closure - read from closure
                auto ofs = m_scope.nonlocal_raw_offset(sym.index(), v.ti);
                // COPY <frame_offset>
                code().add_L2(Opcode::Copy,
                              ofs, v.ti.size());
                v.ti.foreach_heap_slot([this](size_t offset) {
                    // INC_REF <stack_offset>
                    code().add_L1(Opcode::IncRef, offset);
                });
                // if it's a function object, execute it
                if (sym.is_callable()) {
                    code().add(Opcode::Execute);
                }
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
                auto closure_size = function().raw_size_of_nonlocals();
                const auto& ti = function().parameter(sym.index());
                code().add_L2(Opcode::Copy,
                        function().parameter_offset(sym.index()) + closure_size,
                        ti.size());
                ti.foreach_heap_slot([this](size_t offset) {
                    code().add_L1(Opcode::IncRef, offset);
                });
                if (m_callable && ti.is_callable()) {
                    // EXECUTE
                    code().add(Opcode::Execute);
                }
                break;
            }
            case Symbol::Method: {
                assert(v.index != no_index);
                Scope& scope = v.module->get_scope(v.index);
                // this module
                if (v.module == &module()) {
                    // specialization might not be compiled yet - compile it now
                    Function& fn = scope.function();
                    if (fn.is_generic()) {
                        assert(!fn.has_any_generic());  // fully specialized
                        auto body = fn.yank_generic_body();
                        fn.set_assembly();  // this would release AST copy
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
                Scope& scope = v.module->get_scope(v.index);
                // this module
                if (v.module == &module()) {
                    // specialization might not be compiled yet - compile it now
                    Function& fn = scope.function();
                    if (fn.is_generic()) {
                        assert(!fn.has_any_generic());  // fully specialized
                        auto body = fn.yank_generic_body();
                        fn.set_assembly();  // this would release AST copy
                        m_compiler.compile_function(scope, body.ast());
                    }

                    const bool execute = m_callable || !fn.has_nonvoid_parameter();
                    if (scope.has_nonlocals()) {
                        make_closure(scope);
                        // MAKE_CLOSURE <function_idx>
                        code().add_L1(Opcode::MakeClosure, scope.function_index());
                        // EXECUTE
                        if (execute)
                            code().add(Opcode::Execute);
                    } else {
                        if (!execute) {
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
                assert(v.ti.is_function());
                assert(v.ti.signature().param_type.underlying().is_struct());
                const TypeInfo& struct_type = v.ti.signature().param_type.underlying();

                // return the item -> drop all other items from stack, leaving only the selected one
                size_t drop_before = 0;  // first DROP 0 <size>
                size_t skip = 0;
                size_t drop = 0;  // second DROP <skip> <size>
                for (const auto& item : struct_type.subtypes()) {
                    if (item.key() == sym.name()) {
                        std::swap(drop_before, drop);
                        skip = item.size();
                        continue;
                    }
                    item.foreach_heap_slot([this, skip](size_t offset) {
                        // DEC_REF <offset>
                        code().add_L1(Opcode::DecRef, offset + skip);
                    });
                    drop += item.size();
                }
                if (drop_before != 0) {
                    // DROP <skip> <size>
                    code().add_L2(Opcode::Drop, 0, drop_before);
                }
                if (drop != 0) {
                    // DROP <skip> <size>
                    code().add_L2(Opcode::Drop, skip, drop);
                }
                break;
            }
            case Symbol::Unresolved:
                XCI_UNREACHABLE;
        }
    }

    void visit(ast::Call& v) override {
        // call the function or push the function as value

        const bool orig_callable = m_callable;
        m_intrinsic = v.intrinsic;
        m_instruction_args.clear();

        m_callable = false;
        if (v.arg)
            v.arg->apply(*this);

        m_callable = true;
        v.callable->apply(*this);

        // add executes for each call that results in function which consumes more args
        if (v.wrapped_execs > 1) {
            // Emit only a single EXECUTE if there is no closure in the wrapped function calls
            const Signature* sig = &v.callable->type_info().signature();
            for (auto i = v.wrapped_execs; i != 0; --i)
                sig = &sig->return_type.signature();
            if (!sig->has_closure())
                v.wrapped_execs = 1;
        }
        for (auto i = v.wrapped_execs; i != 0; --i)
            code().add(Opcode::Execute);

        m_intrinsic = false;
        m_callable = orig_callable;
    }

    void visit(ast::OpCall& v) override {
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        // See "Conditional jumps" in `docs/script/machine.adoc`
        std::vector<size_t> end_labels;
        for (auto& item : v.if_then_expr) {
            // condition
            item.first->apply(*this);
            // JUMP_IF_NOT (to next condition)
            const auto label_cond = code().add_label();
            code().add_L2(Opcode::Annotation, (size_t) CodeAssembly::Annotation::JumpIfNot, label_cond);
            // then branch
            item.second->apply(*this);
            // JUMP (to end)
            const auto label_end = code().add_label();
            end_labels.push_back(label_end);
            code().add_L2(Opcode::Annotation, (size_t) CodeAssembly::Annotation::Jump, label_end);
            // add label for the above JUMP_IF_NOT
            code().add_L2(Opcode::Annotation, (size_t) CodeAssembly::Annotation::Label, label_cond);
        }
        // else branch
        v.else_expr->apply(*this);
        // add end labels for all previous JUMP instructions
        for (const auto label : end_labels) {
            code().add_L2(Opcode::Annotation, (size_t) CodeAssembly::Annotation::Label, label);
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
                v.type_info().effective_type().size(), v.leave_type.size());
        // call the leave function - must pull the context from enter function
        v.leave_function.apply(*this);
    }

    void visit(ast::Function& v) override {
        // compile body
        Scope& scope = module().get_scope(v.scope_index);
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

        if (!func.is_assembly()) {
            func.set_assembly();
            m_compiler.compile_function(scope, v.body);
        }

        if (scope.has_nonlocals()) {
            if (v.definition) {
                /*if (!func.has_nonvoid_parameters()) {
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
                if (!func.has_nonvoid_parameter()) {
                    // parameterless closure is executed immediately
                    // EXECUTE
                    code().add(Opcode::Execute);
                }
            }
        } else if (!v.definition) {
            // call the function only if it's inside a Call which applies all required parameters (might be zero)
            if (v.call_arg >= func.has_nonvoid_parameter()) {
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
        if (v.cast_function) {
            const bool orig_callable = m_callable;
            m_callable = true;
            v.cast_function->apply(*this);
            m_callable = orig_callable;
        }
    }

    void visit(ast::Instance& v) override {
        for (auto& dfn : v.defs)
            dfn.apply(*this);
    }

    void visit(ast::Class& v) override {}

private:
    Module& module() { return m_scope.module(); }
    Function& function() { return m_scope.function(); }
    CodeAssembly& code() { return function().asm_code(); }

    void make_closure(const Scope& scope) {
        if (!scope.has_nonlocals())
            return;
        const auto& func = scope.function();
        // parent function
        assert(scope.parent() != nullptr);
        auto& parent_scope = *scope.parent();
        auto& parent_fn = parent_scope.function();
        auto closure_size = parent_fn.raw_size_of_nonlocals();
        // make closure
        auto nl_i = scope.nonlocals().size();
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
                    const Function& fn = module().get_function(fn_idx);
                    if (subscope.has_nonlocals()) {
                        make_closure(subscope);
                        // MAKE_CLOSURE <function_idx>
                        code().add_L1(Opcode::MakeClosure, fn_idx);
                    } else if (fn.has_nonvoid_parameter()) {
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

private:
    Compiler& m_compiler;
    Scope& m_scope;

    bool m_callable = false;

    // intrinsics
    bool m_intrinsic = false;
    std::vector<TypedValue> m_instruction_args;
};


using FnCallback = void (*)(Function&);
static void foreach_asm_fn_in_module(Module& module, FnCallback cb)
{
    for (unsigned i = module.num_scopes(); i != 0; --i) {
        const Scope& scope = module.get_scope(i - 1);
        if (!scope.has_function())
            continue;
        Function& fn = scope.function();
        if (!fn.is_assembly())
            continue; // generic

        cb(fn);
    }
}


void Compiler::compile(Scope& scope, ast::Module& ast)
{
    auto& func = scope.function();
    func.set_assembly();
    func.set_compile();
    func.signature().set_parameter(ti_void());
    func.signature().set_return_type(ti_unknown());
    ast.body.symtab = &func.symtab();

    // Preprocess AST
    // - resolve symbols (SymbolResolver)
    // - infer and check types (TypeResolver)
    // - apply optimizations - const fold etc. (Optimizer)
    // See documentation on each function.

    if ((m_flags & Flags::FoldTuple) == Flags::FoldTuple)
        fold_tuple(ast.body);

    if ((m_flags & Flags::FoldDotCall) == Flags::FoldDotCall)
        fold_dot_call(ast.body);

    if ((m_flags & Flags::FoldParen) == Flags::FoldParen)
        fold_paren(ast.body);

    if ((m_flags & Flags::ResolveSymbols) == Flags::ResolveSymbols)
        resolve_symbols(scope, ast.body);

    if ((m_flags & Flags::ResolveDecl) == Flags::ResolveDecl)
        resolve_decl(scope, ast.body);

    if ((m_flags & Flags::ResolveTypes) == Flags::ResolveTypes)
        resolve_types(scope, ast.body);

    if ((m_flags & Flags::ResolveSpec) == Flags::ResolveSpec)
        resolve_spec(scope, ast.body);

    if ((m_flags & Flags::ResolveNonlocals) == Flags::ResolveNonlocals)
        resolve_nonlocals(scope, ast.body);

    if ((m_flags & Flags::CompileFunctions) == Flags::CompileFunctions)
        compile_all_functions(scope);

    if ((m_flags & Flags::FoldConstExpr) == Flags::FoldConstExpr)
        fold_const_expr(func, ast.body);

    if ((m_flags & Flags::CompileFunctions) == Flags::CompileFunctions)
        compile_function(scope, ast.body);

    // TODO
//    if ((m_flags & Flags::InlineFunctions) == Flags::InlineFunctions)
//        inline_functions(scope);

    if ((m_flags & Flags::OptimizeCopyDrop) == Flags::OptimizeCopyDrop)
        foreach_asm_fn_in_module(scope.module(), optimize_copy_drop);

    if ((m_flags & Flags::OptimizeTailCall) == Flags::OptimizeTailCall)
        foreach_asm_fn_in_module(scope.module(), optimize_tail_call);

    if ((m_flags & Flags::AssembleFunctions) == Flags::AssembleFunctions)
        foreach_asm_fn_in_module(scope.module(), [](Function& fn){ fn.assembly_to_bytecode(); });
}


void Compiler::compile_function(Scope& scope, ast::Expression& body)
{
    Function& fn = scope.function();
    const auto parameter_size = fn.raw_size_of_parameter();
    const auto closure_size = fn.raw_size_of_nonlocals();

    if (fn.is_expression() && fn.has_nonvoid_parameter()) {
        // Copy parameter to be passed to a function contained inside the expression
        fn.asm_code().add_L2(Opcode::Copy, closure_size, parameter_size);
        fn.parameter().foreach_heap_slot([&fn](size_t offset) {
            fn.asm_code().add_L1(Opcode::IncRef, offset);
        });
    }

    // Compile AST into bytecode
    CompilerVisitor visitor(*this, scope);
    body.apply(visitor);

    if (fn.has_intrinsics()) {
        if (fn.intrinsics() != fn.asm_code().size())
            throw intrinsics_function_error(
                    "cannot mix compiled code with intrinsics",
                    body.source_loc);
        // RET, do not add any DROP for intrinsics function
        fn.asm_code().add(Opcode::Ret);
        return;
    }

    auto skip = fn.signature().return_type.effective_type().size();
    auto drop = parameter_size + closure_size;
    if (drop > 0) {
        Stack::StackRel pos = skip;
        for (const auto& ti : fn.nonlocals()) {
            ti.foreach_heap_slot([&fn, pos](size_t offset) {
                // DEC_REF <addr in nonlocals>
                fn.asm_code().add_L1(Opcode::DecRef, pos + offset);
            });
            pos += ti.size();
        }

        fn.parameter().foreach_heap_slot([&fn, pos](size_t offset) {
            // DEC_REF <addr in params>
            fn.asm_code().add_L1(Opcode::DecRef, pos + offset);
        });
        // DROP <ret_value> <params + nonlocals>
        fn.asm_code().add_L2(Opcode::Drop, skip, drop);
    }
    // RET (return value left on stack)
    fn.asm_code().add(Opcode::Ret);
}


void Compiler::compile_all_functions(Scope& main)
{
    Module& module = main.module();
    for (unsigned i = module.num_scopes(); i != 0; --i) {
        Scope& scope = module.get_scope(i - 1);
        if (&scope == &main || !scope.has_function())
            continue;
        Function& fn = scope.function();
        if (!fn.is_generic())
            continue; // already compiled
        if (!fn.has_compile()) {
            // keep generic, make sure AST is copied
            assert(fn.has_any_generic() || scope.has_unresolved_type_params());
            fn.ensure_ast_copy();
            continue;
        }

        assert(!fn.has_any_generic());
        auto body = fn.yank_generic_body();
        fn.set_assembly();  // this removes AST from the function
        compile_function(scope, body.ast());
    }
}


} // namespace xci::script
