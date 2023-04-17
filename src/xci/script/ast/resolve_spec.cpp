// resolve_spec.cpp created on 2022-08-13 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "resolve_spec.h"
#include <xci/script/typing/TypeChecker.h>
#include <xci/script/typing/OverloadResolver.h>
#include <xci/script/typing/GenericResolver.h>
#include <xci/script/Builtin.h>
#include <xci/script/Function.h>
#include <xci/script/Error.h>
#include <xci/script/dump.h>
#include <xci/compat/macros.h>
#include <xci/core/log.h>

#include <range/v3/view/enumerate.hpp>

#include <sstream>
#include <optional>

namespace xci::script {

using std::stringstream;
using namespace xci::core;


class ResolveSpecVisitor final: public ast::VisitorExclTypes {
public:
    using VisitorExclTypes::visit;

    explicit ResolveSpecVisitor(Scope& scope) : m_scope(scope) {}

    void visit(ast::Definition& dfn) override {
        if (dfn.expression) {
            dfn.expression->apply(*this);

            Function& fn = dfn.symbol().get_function(m_scope);
            if (m_value_type.is_callable())
                fn.signature() = m_value_type.signature();
            else {
                const auto& source_loc = dfn.expression ?
                                dfn.expression->source_loc : dfn.variable.identifier.source_loc;
                resolve_return_type(fn.signature(), m_value_type,
                                    dfn.symbol().get_scope(m_scope), source_loc);
            }
        }

        m_value_type = {};
    }

    void visit(ast::Invocation& inv) override {
        inv.expression->apply(*this);
        auto res_type = m_value_type.effective_type();
        // Unknown in intrinsics function
        if (!res_type.is_void() && !res_type.is_unknown())
            inv.type_id = get_type_id(std::move(res_type));
    }

    void visit(ast::Return& ret) override {
        ret.expression->apply(*this);
        resolve_return_type(function().signature(), m_value_type, m_scope,
                            ret.expression->source_loc);
    }

    void visit(ast::Class& v) override {
        for (auto& dfn : v.defs)
            dfn.apply(*this);
    }

    void visit(ast::Instance& v) override {
        for (auto& dfn : v.defs)
            dfn.apply(*this);
    }

    void visit(ast::Literal& v) override {
        m_value_type = v.ti;
    }

    void visit(ast::Tuple& v) override {
        for (auto& item : v.items) {
            item->apply(*this);
        }
        m_value_type = v.ti;
    }

    void visit(ast::List& v) override {
        TypeChecker type_check(std::move(v.ti), std::move(m_cast_type));
        // check all items have same type
        TypeInfo elem_type;
        if (!type_check.eval_type() && v.items.empty())
            elem_type = ti_void();
        else for (auto& item : v.items) {
                item->apply(*this);
                if (item.get() == v.items.front().get()) {
                    // first item
                    elem_type = std::move(m_value_type);
                } else {
                    // other items
                    if (elem_type != m_value_type)
                        throw ListElemTypeMismatch(elem_type, m_value_type, item->source_loc);
                }
            }
        m_value_type = type_check.resolve(ti_list(std::move(elem_type)), v.source_loc);
        assert(m_value_type.is_list());
        if (m_value_type.elem_type().is_unknown() && type_check.eval_type())
            m_value_type = std::move(type_check.eval_type());
        // FIXME: allow generic type: fun <T> Void->[T] { []:[T] }
        if (m_value_type.elem_type().is_generic())
            throw MissingExplicitType(v.source_loc);
        v.ti = m_value_type;
        v.elem_type_id = get_type_id(m_value_type.elem_type());
    }

    void visit(ast::StructInit& v) override {
        // first pass - resolve incomplete struct type
        //              and check it matches specified type (if any)
        TypeChecker type_check(std::move(v.ti), std::move(m_cast_type));
        const auto& specified = type_check.eval_type();
        if (!specified.is_unknown() && !specified.is_struct())
            throw StructTypeMismatch(specified, v.source_loc);
        // build TypeInfo for the struct initializer
        TypeInfo::StructItems ti_items;
        ti_items.reserve(v.items.size());
        for (auto& item : v.items) {
            // resolve item type
            if (specified) {
                const TypeInfo* specified_item = specified.struct_item_by_name(item.first.name);
                if (specified_item)
                    m_type_info = *specified_item;
            }
            item.second->apply(*this);
            m_type_info = {};
            auto item_type = m_value_type.effective_type();
            if (!specified.is_unknown())
                type_check.check_struct_item(item.first.name, item_type, item.second->source_loc);
            ti_items.emplace_back(item.first.name, item_type);
        }
        v.ti = TypeInfo(std::move(ti_items));
        if (!specified.is_unknown()) {
            assert(match_struct(v.ti, specified));  // already checked above
            v.ti = std::move(type_check.eval_type());
        }
        m_value_type = v.ti;

        // Add the inferred struct type to module, point StructItem symbols to it
        const Index index = module().add_type(v.ti);
        for (auto& item : v.items) {
            item.first.symbol->set_index(index);
        }
    }

    void visit(ast::Reference& v) override {
        assert(v.identifier.symbol);
        const auto& symtab = *v.identifier.symbol.symtab();
        const auto& sym = *v.identifier.symbol;

        // referencing variable / function - not a literal value, in case this is Call arg
        m_literal_value = false;

        switch (sym.type()) {
            case Symbol::Instruction:
                m_call_sig.clear();
                break;
            case Symbol::TypeId: {
                if (v.ti.is_unknown()) {
                    // try to resolve via known type args
                    auto var = v.ti.generic_var();
                    const auto& type_args = m_scope.type_args();
                    auto resolved = type_args.get(var);
                    if (resolved) {
                        v.ti = resolved;
                    } else {
                        // unresolved -> unknown type id
                        m_value_type = {};
                        return;
                    }
                }
                // Record the resolved Type ID for Compiler
                v.index = get_type_id(v.ti);
                m_value_type = ti_int32();
                return;  // do not overwrite m_value_type below
            }
            case Symbol::Class:
            case Symbol::Instance:
                break;
            case Symbol::Method: {
                if (v.definition) {
                    const Function& fn = v.definition->symbol().get_function(m_scope);
                    m_call_sig.load_from(fn.signature(), v.source_loc);
                }

                // find prototype of the function, resolve actual type of T
                const auto& symmod = symtab.module() == nullptr ? module() : *symtab.module();
                auto& cls = symmod.get_class(sym.index());
                const Index cls_fn_idx = cls.get_index_of_function(sym.ref()->index());
                const auto& cls_fn = sym.ref().get_generic_scope().function();
                auto inst_types = resolve_instance_types(cls_fn.signature());
                std::vector<TypeInfo> resolved_types;
                for (Index i = 1; i <= cls.symtab().count(Symbol::TypeVar); ++i) {
                    auto symptr = cls.symtab().find_by_index(Symbol::TypeVar, i);
                    TypeInfo ti;
                    get_type_arg(symptr, ti, inst_types);
                    resolved_types.push_back(ti);
                }

                // find instance using resolved T
                std::vector<Candidate> candidates;
                for (auto inst_psym : v.sym_list) {
                    assert(inst_psym->type() == Symbol::Instance);
                    auto* inst_mod = inst_psym.symtab()->module();
                    if (inst_mod == nullptr)
                        inst_mod = &module();
                    auto& inst = inst_mod->get_instance(inst_psym->index());
                    auto inst_fn = inst.get_function(cls_fn_idx);
                    auto m = match_params(inst.types(), resolved_types);
                    candidates.push_back({inst_mod, inst_fn.scope_index, inst_psym, TypeInfo{}, {}, m});
                }

                auto [found, conflict] = find_best_candidate(candidates);

                if (found && !conflict) {
                    auto spec_idx = specialize_instance(found->symptr, cls_fn_idx, v.identifier.source_loc);
                    if (spec_idx != no_index) {
                        auto& inst = module().get_instance(spec_idx);
                        auto inst_scope_idx = inst.get_function(cls_fn_idx).scope_index;
                        v.module = &module();
                        v.index = inst_scope_idx;
                        auto& scope = v.module->get_scope(v.index);
                        scope.type_args().add_from(inst_types);
                    } else {
                        v.module = found->module;
                        v.index = found->scope_index;
                    }
                    auto& fn = v.module->get_scope(v.index).function();
                    m_value_type = TypeInfo{fn.signature_ptr()};
                    break;
                }

                // ERROR couldn't find single matching instance for `args`
                stringstream o_candidates;
                for (const auto& c : candidates) {
                    auto& fn = c.module->get_scope(c.scope_index).function();
                    o_candidates << "   " << c.match << "  "
                                 << fn.signature() << std::endl;
                }
                stringstream o_ftype;
                o_ftype << v.identifier.name << ' ' << m_call_sig.signature();
                if (conflict)
                    throw FunctionConflict(o_ftype.str(), o_candidates.str(), v.source_loc);
                else
                    throw FunctionNotFound(o_ftype.str(), o_candidates.str(), v.source_loc);
            }
            case Symbol::Function:
            case Symbol::StructItem: {
                // specified type in definition
                if (sym.type() == Symbol::Function && v.definition && v.ti) {
                    assert(m_call_sig.empty());
                    if (v.ti.is_callable()) {
                        m_call_sig.load_from(v.ti.signature(), v.source_loc);
                    } else {
                        // A naked type, consider it a function return type
                        m_call_sig.return_type = v.ti;
                    }
                }

                // Specialize
                if (sym.type() == Symbol::Function) {
                    auto& scope = v.module->get_scope(v.index);
                    auto& fn = scope.function();
                    if (!fn.is_specialized()) {
                        auto specialized = specialize_function(v.identifier.symbol, v.identifier.source_loc, v.type_args_ti);
                        if (specialized) {
                            v.module = &module();
                            v.index = specialized.scope_index;
                            v.ti = std::move(specialized.type_info);
                        } else if (v.ti.is_generic() && !fn.has_any_generic()) {
                            v.ti = TypeInfo(fn.signature_ptr());
                        }
                    }
                    if (v.definition) {
                        m_call_sig.clear();
                    }
                } else {
                    assert(sym.type() == Symbol::StructItem);
                    m_call_sig.clear();
                }
                break;
            }
            case Symbol::Value:
            case Symbol::Module:
            case Symbol::Parameter: {
                const auto* ref_scope = m_scope.find_parent_scope(&symtab);
                if (ref_scope) {
                    const auto& sig_type = ref_scope->function().parameter(sym.index());
                    if (sig_type.is_callable() && sig_type.is_generic()) {
                        auto call_type_args = specialize_signature(sig_type.signature_ptr(), m_call_sig);
                        m_scope.type_args().add_from(call_type_args);
                    }
                    v.ti = sig_type;
                }
                break;
            }
            case Symbol::TypeName:
            case Symbol::TypeVar:
                break;
            case Symbol::Nonlocal:
            case Symbol::Unresolved:
                XCI_UNREACHABLE;
        }
        m_value_type = v.ti;
        if (m_value_type.is_generic())
            resolve_generic_type(m_value_type, m_scope);
    }

    void visit(ast::Call& v) override {
        if (v.definition) {
            Function& fn = v.definition->symbol().get_function(m_scope);
            if (fn.signature().params.empty())
                m_type_info = fn.signature().return_type;
            else
                m_type_info = TypeInfo{fn.signature_ptr()};
        }

        TypeChecker type_check(std::move(m_type_info), std::move(m_cast_type));

        // resolve each argument
        std::vector<CallArg> call_args;
        auto orig_call_sig = std::move(m_call_sig);
        for (auto& arg : v.args) {
            m_call_sig.clear();
            m_literal_value = true;
            arg->apply(*this);
            assert(arg->source_loc);
            call_args.push_back({m_value_type.effective_type(), arg->source_loc, m_literal_value});
        }
        // append args to m_call_args (note that m_call_args might be used
        // when evaluating each argument, so we cannot push to them above)
        m_call_sig = std::move(orig_call_sig);
        std::move(call_args.begin(), call_args.end(), std::back_inserter(m_call_sig.args));
        m_call_sig.return_type = std::move(type_check.eval_type());
        m_literal_value = false;

        // using resolved args, resolve the callable itself
        // (it may use args types for overload resolution)
        v.callable->apply(*this);

        if (m_value_type.is_callable()) {
            // result is new signature with args removed (applied)
            const auto param_type_args = resolve_generic_args_to_signature(m_value_type.signature(), m_call_sig);
            store_resolved_param_type_vars(m_scope, param_type_args);
            auto new_signature = consume_params_from_call_args(m_value_type.signature_ptr(), v);
            if (new_signature->params.empty()) {
                if (v.definition == nullptr) {
                    // all args consumed, or a zero-arg function being called
                    // -> effective type is the return type
                    v.ti = new_signature->return_type;
                } else {
                    // Not really calling, just defining, e.g. `f = compose u v`
                    // Keep the return type as is, making it `() -> <lambda type>`
                    v.ti = TypeInfo{new_signature};
                }
                v.partial_args = 0;
            } else {
                if (v.partial_args != 0) {
                    // partial function call
                    if (v.definition != nullptr) {
                        v.partial_index = v.definition->symbol().get_scope_index(m_scope);
                    } else {
                        SymbolTable& fn_symtab = function().symtab().add_child("?/partial");
                        Function fn {module(), fn_symtab};
                        auto fn_idx = module().add_function(std::move(fn)).index;
                        v.partial_index = module().add_scope(Scope{module(), fn_idx, &m_scope});
                        m_scope.add_subscope(v.partial_index);
                    }
                    auto& fn = module().get_scope(v.partial_index).function();
                    fn.signature() = *new_signature;
                    fn.signature().nonlocals.clear();
                    fn.signature().partial.clear();
                    for (const auto& arg : m_call_sig.args) {
                        fn.add_partial(TypeInfo{arg.type_info});
                    }
                    assert(!fn.has_any_generic());
                    fn.set_compile();
                }
                v.ti = TypeInfo{new_signature};
            }
        }

        // Second pass of args, now with resolved types
        // (if a generic function was passed in args, it can be specialized now)
        call_args = std::move(m_call_sig.args);
        unsigned i = 0;
        for (auto& arg : v.args) {
            if (i >= call_args.size())
                break;
            auto call_ti = std::move(call_args[i++].type_info);
            if (call_ti.is_callable()) {
                m_call_sig.load_from(call_ti.signature(), arg->source_loc);
                arg->apply(*this);
            }
        }

        m_call_sig.clear();
        m_value_type = v.ti;
        if (m_value_type.is_generic())
            resolve_generic_type(m_value_type, m_scope);
    }

    void visit(ast::OpCall& v) override {
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        for (auto& item : v.if_then_expr) {
            item.first->apply(*this);
            item.second->apply(*this);
        }
        v.else_expr->apply(*this);
        m_literal_value = false;
    }

    void visit(ast::WithContext& v) override {
        // resolve type of context (StructInit leads to incomplete struct type)
        m_literal_value = true;
        v.context->apply(*this);
        // lookup the enter function with the resolved context type
        m_call_sig.add_arg({m_value_type, v.context->source_loc, m_literal_value});
        m_call_sig.return_type = ti_unknown();
        v.enter_function.apply(*this);
        m_call_sig.args.clear();
        assert(m_value_type.is_callable());
        auto enter_sig = m_value_type.signature();
        // re-resolve type of context (match actual struct type as found by resolving `with` function)
        m_cast_type = enter_sig.params[0];
        m_literal_value = true;
        v.context->apply(*this);
        assert(m_value_type == enter_sig.params[0]);
        // lookup the leave function, it's arg type is same as enter functions return type
        v.leave_type = enter_sig.return_type.effective_type();
        m_call_sig.add_arg({v.leave_type, v.context->source_loc, m_literal_value});
        m_call_sig.return_type = ti_void();
        v.leave_function.apply(*this);
        m_call_sig.clear();
        // resolve type of expression - it's also the type of the whole "with" expression
        v.expression->apply(*this);
        m_literal_value = false;
    }

    void visit(ast::Function& v) override {
        if (v.symbol->type() == Symbol::Function) {
            v.scope_index = v.symbol.get_scope_index(m_scope);
        }
        auto& scope = module().get_scope(v.scope_index);

        // This is a nested function in a function we're currently specializing.
        // Make sure we work on specialized nested function, not original (generic) one.
        const bool parent_is_specialized = scope.parent()->has_function() &&
                                           scope.parent()->function().is_specialized();
        if (parent_is_specialized) {
            assert(!scope.function().is_specialized());
            auto clone_fn_idx = clone_function(scope);
            auto& clone_fn = module().get_function(clone_fn_idx);
            clone_fn.ensure_ast_copy();
            scope.set_function_index(clone_fn_idx);
        }

        Function& fn = scope.function();

        m_value_type = TypeInfo{fn.signature_ptr()};
        m_literal_value = false;
        m_value_type = v.ti;

        if (parent_is_specialized) {
            if (!v.definition) {
                fn.set_specialized();
                specialize_to_call_args(scope, fn.ast(), v.source_loc);
                if (fn.has_any_generic()) {
                    stringstream sig_str;
                    sig_str << fn.name() << ':' << fn.signature();
                    throw UnexpectedGenericFunction(sig_str.str(), v.source_loc);
                }
                m_value_type = TypeInfo{fn.signature_ptr()};
            }
        } else if (fn.has_generic_params() || scope.has_unresolved_type_params()) {
            if (!v.definition) {
                // immediately called or returned generic function
                // -> try to instantiate the specialization
                auto clone_fn_idx = clone_function(scope);
                auto& clone_fn = module().get_function(clone_fn_idx);
                clone_fn.set_specialized();
                scope.set_function_index(clone_fn_idx);
                specialize_to_call_args(scope, clone_fn.ast(), v.source_loc);
                if (clone_fn.has_any_generic()) {
                    stringstream sig_str;
                    sig_str << clone_fn.name() << ':' << clone_fn.signature();
                    throw UnexpectedGenericFunction(sig_str.str(), v.source_loc);
                }
                m_value_type = TypeInfo{clone_fn.signature_ptr()};
            }/* else {
                resolve_spec(scope, v.body);
                m_value_type = TypeInfo{fn.signature_ptr()};
            }*/
        } else {
            // compile body and resolve return type
            if (v.definition) {
                // in case the function is recursive, propagate the type upwards
                auto symptr = v.definition->symbol();
                auto& fn_dfn = symptr.get_function(m_scope);
                fn_dfn.set_signature(m_value_type.signature_ptr());
            }
            resolve_spec(scope, v.body);
            m_value_type = TypeInfo{fn.signature_ptr()};
        }

        if (m_value_type.is_generic())
            resolve_generic_type(m_value_type, m_scope);
        v.ti = m_value_type;
    }

    // The cast expression is translated to a call to `cast` method from the Cast class.
    // The inner expression type and the cast type are used to look up the instance of Cast.
    void visit(ast::Cast& v) override {
        // resolve the inner expression -> m_value_type
        // (the Expression might use the specified type from `m_cast_type`)
        resolve_generic_type(v.to_type, m_scope.type_args());
        m_cast_type = v.to_type;
        m_literal_value = true;
        v.expression->apply(*this);
        m_cast_type = {};
        m_value_type = m_value_type.effective_type();
        // Cast to the same type or same underlying type (from/to a named type) -> noop
        if (is_same_underlying(m_value_type, v.to_type)) {
            v.cast_function.reset();
            m_value_type = v.to_type;
            return;
        }
        // lookup the cast function with the resolved arg/return types
        m_call_sig.add_arg({m_value_type, v.expression->source_loc, m_literal_value});
        m_call_sig.return_type = v.to_type;
        v.cast_function->apply(*this);
        // set the effective type of the Cast expression and clean the call types
        m_value_type = std::move(m_call_sig.return_type);
        m_literal_value = false;
        m_call_sig.clear();
    }

private:
    Module& module() const { return m_scope.module(); }
    Function& function() const { return m_scope.function(); }

    Index get_type_id(TypeInfo&& type_info) {
        // is the type builtin?
        const Module& builtin_module = module().module_manager().builtin_module();
        Index type_id = builtin_module.find_type(type_info);
        if (type_id >= 32) {
            // add to current module
            type_id = 32 + module().add_type(std::move(type_info));
        }
        return type_id;
    }

    Index get_type_id(const TypeInfo& type_info) {
        // is the type builtin?
        const Module& builtin_module = module().module_manager().builtin_module();
        Index type_id = builtin_module.find_type(type_info);
        if (type_id >= 32) {
            // add to current module
            type_id = 32 + module().add_type(type_info);
        }
        return type_id;
    }

    // Check return type matches and set it to concrete type if it's generic.
    void resolve_return_type(Signature& sig, const TypeInfo& deduced,
                             Scope& scope, const SourceLocation& loc) const
    {
        if (sig.return_type.is_unknown() || sig.return_type.is_generic()) {
            if (deduced.is_unknown() && !deduced.is_generic()) {
                if (!sig.has_any_generic())
                    throw MissingExplicitType(loc);
                return;  // nothing to resolve
            }
            if (deduced.is_callable() && &sig == &deduced.signature())
                throw MissingExplicitType(loc);  // the return type is recursive!
            specialize_arg(sig.return_type, deduced, scope.type_args(),
                    [](const TypeInfo& exp, const TypeInfo& got) {
                        throw UnexpectedReturnType(exp, got);
                    });
            resolve_type_vars(sig, scope.type_args());  // fill in concrete types using new type var info
            sig.return_type = deduced;  // Unknown/var=0 not handled by resolve_type_vars
            return;
        }
        if (sig.return_type != deduced)
            throw UnexpectedReturnType(sig.return_type, deduced);
    }

    // Consume params from `signature` according to `m_call_args`, creating new signature
    SignaturePtr consume_params_from_call_args(const SignaturePtr& signature, ast::Call& v)
    {
        auto res = std::make_shared<Signature>(*signature);  // a copy to work on (modified below)
        const TypeArgs call_type_args = specialize_signature(signature, m_call_sig);
        int i = 0;
        v.partial_args = 0;
        v.wrapped_execs = 0;
        for (const auto& arg : m_call_sig.args) {
            ++i;
            // check there are more params to consume
            while (res->params.empty()) {
                assert(res->return_type.type() == Type::Function);  // checked by specialize_signature() above
                // collapse returned function, start consuming its params
                res = std::make_shared<Signature>(res->return_type.signature());
                ++v.wrapped_execs;
                v.partial_args = 0;
            }
            // check type of next param
            const auto& sig_param = res->params.front();
            const auto m = match_type(arg.type_info, sig_param);
            if (!(m.is_exact() || m.is_generic() || (m.is_coerce() && arg.literal_value))) {
                throw UnexpectedArgumentType(i, sig_param, arg.type_info, arg.source_loc);
            }
            if (m.is_coerce()) {
                // Update type_info of the coerced literal argument
                m_cast_type = sig_param;
                v.args[i - 1]->apply(*this);
            }
            if (sig_param.is_callable()) {
                // resolve overload in case the arg is a function that was specialized
                auto orig_call_sig = std::move(m_call_sig);
                m_call_sig.load_from(sig_param.signature(), arg.source_loc);
                v.args[i - 1]->apply(*this);
                m_call_sig = std::move(orig_call_sig);
            }
            // consume next param
            ++ v.partial_args;
            res->params.erase(res->params.begin());
        }
        resolve_type_vars(*res, call_type_args);
        return res;
    }

    // Specialize a generic function:
    // * use m_call_args to resolve actual types of type variable
    // * resolve function body (deduce actual return type)
    // * use the deduced return type to resolve type variables in generic return type
    // Modifies `fn` in place - it should be already copied.
    // Throw when the signature doesn't match the call args or deduced return type.
    void specialize_to_call_args(Scope& scope, const ast::Block& body, const SourceLocation& loc,
                                 TypeArgs type_args = {})
    {
        log::debug("Specialize '{}' to {}\nType args: {}",
                   scope.function().name(), m_call_sig.signature(), scope.type_args());

//        if (scope.parent() != &m_scope)
//            scope.type_args().add_from(m_scope.type_args());

        {
            auto sig = scope.function().signature_ptr();
            resolve_type_vars(*sig, scope);
            const TypeArgs call_type_args = specialize_signature(sig, m_call_sig, std::move(type_args));
            // resolve generic vars to received types
            resolve_type_vars(*sig, call_type_args);
            scope.type_args().add_from(call_type_args);
        }

        auto& signature = scope.function().signature();
        // resolve function body to get actual return type
        auto sig_ret = signature.return_type;
        resolve_spec(scope, body);
        auto deduced_ret = signature.return_type;
        // resolve generic return type
        if (!deduced_ret.is_unknown() && deduced_ret != sig_ret)
            specialize_arg(sig_ret, deduced_ret, scope.type_args(),
                           [](const TypeInfo& exp, const TypeInfo& got) {
                               throw UnexpectedReturnType(exp, got);
                           });
        resolve_generic_type(sig_ret, scope.type_args());
        signature.return_type = sig_ret;
    }

    struct Specialized {
        TypeInfo type_info;
        Index scope_index = no_index;
        operator bool() const noexcept { return scope_index != no_index; }
    };

    Index clone_function(const Scope& scope) const
    {
        const auto& fn = scope.function();
        auto clone_fn_idx = module().add_function(Function(module(), fn.symtab())).index;
        auto& clone_fn = module().get_function(clone_fn_idx);
        auto clone_sig = std::make_shared<Signature>(fn.signature());  // copy, not ref
        clone_fn.set_signature(clone_sig);
        if (fn.is_generic()) {
            clone_fn.set_ast(fn.ast());
        }
        return clone_fn_idx;
    }

    Index clone_scope(const Scope& scope, Index fn_idx) const
    {
        auto fscope_idx = module().add_scope(Scope{module(), fn_idx, scope.parent()});
        auto& fscope = module().get_scope(fscope_idx);
        fscope.copy_subscopes(scope);
        return fscope_idx;
    }

    /// Given a generic function, create a copy and specialize it to call args.
    /// * create a copy of original generic function in this module
    /// * copy function's AST
    /// * keep original symbol table (with relative references, like parameter #1 at depth -2)
    /// Symbols in copied AST still point to original generic function.
    /// \param symptr   Pointer to symbol pointing to original function
    /// \returns TypeInfo and Index of the specialized function in this module
    Specialized specialize_function(SymbolPointer symptr, const SourceLocation& loc, const std::vector<TypeInfo>& type_args)
    {
        auto& scope = symptr.get_scope(m_scope);
        auto& fn = scope.function();
        if (m_scope.find_parent_scope(&fn.symtab()))
            return {};  // recursive call - cannot specialize parent function from nested

        const auto& generic_scope = symptr.get_generic_scope();
        const auto& generic_fn = generic_scope.function();

        if (&scope != &generic_scope && !fn.is_specialized()) {
            // This scope is already a clone made for parent specialized function.
            // Reuse it and specialize the function inside.
            if (scope.function_index() == generic_scope.function_index()) {
                auto clone_fn_idx = clone_function(scope);
                auto& clone_fn = module().get_function(clone_fn_idx);
                clone_fn.ensure_ast_copy();
                scope.set_module(module());
                scope.set_function_index(clone_fn_idx);
            }
            scope.type_args().add_from(m_scope.type_args());
            auto& fspec = scope.function();
            fspec.set_specialized();
            specialize_to_call_args(scope, fspec.ast(), loc);
            const auto scope_idx = symptr.get_scope_index(m_scope);
            module().add_spec_function(symptr, scope_idx);
            return {TypeInfo{fspec.signature_ptr()}, scope_idx};
        }

        if (generic_fn.is_specialized())
            return {};  // already specialized
        if (!generic_fn.is_generic() || !(generic_fn.has_any_generic() || generic_scope.has_unresolved_type_params()))
            return {};  // not generic, nothing to specialize
        if (generic_fn.signature().params.size() > m_call_sig.n_args() && generic_fn.signature().has_nonvoid_params())
            return {};  // not enough call args
        if (!function().is_specialized() && type_args.empty()
            && (!scope.parent()->has_function() || !scope.parent()->function().is_specialized()))
        {
            // when not specializing the parent function...
            if (std::all_of(m_call_sig.args.cbegin(), m_call_sig.args.cend(),
                            [](const CallArg& arg) {
                                return arg.type_info.is_generic();
                            }))
                return {};  // do not specialize with generic args
        }

        TypeArgs explicit_type_args;
        if (!type_args.empty()) {
            unsigned i = 0;
            for (auto var : fn.symtab().filter(Symbol::TypeVar)) {
                if (var->name().front() == '$')
                    continue;
                set_type_arg(var, type_args[i], explicit_type_args,
                             [i, &loc](const TypeInfo& exp, const TypeInfo& got)
                             { throw UnexpectedArgumentType(i, exp, got, loc); });
                if (++i >= type_args.size())
                    break;
            }
        }

        // Check already created specializations if one of them matches
        for (auto spec_scope_idx : module().get_spec_functions(symptr)) {
            auto& spec_scope = module().get_scope(spec_scope_idx);
            if (spec_scope.parent() != scope.parent())
                continue;  // the specialization is from a different scope hierarchy
            if (!match_type_args(explicit_type_args, spec_scope.type_args()))
                continue;
            auto& spec_fn = spec_scope.function();
            if (match_signature(spec_fn.signature()).is_exact())
                return {TypeInfo{spec_fn.signature_ptr()}, spec_scope_idx};
        }

        auto fspec_idx = clone_function(generic_scope);
        auto& fspec = module().get_function(fspec_idx);
        fspec.set_specialized();
        fspec.ensure_ast_copy();
        auto fscope_idx = clone_scope(scope, fspec_idx);
        auto& fscope = module().get_scope(fscope_idx);

        // Copy type args from current scope.
        // This is needed when the current scope is not in parent relation to fspec
        // (that happens when the function being specialized was called)
        fscope.type_args().add_from(m_scope.type_args());

        specialize_to_call_args(fscope, fspec.ast(), loc, std::move(explicit_type_args));

        assert(symptr->depth() == 0);
        // add to specialized functions in this module
        module().add_spec_function(symptr, fscope_idx);
        return {TypeInfo{fspec.signature_ptr()}, fscope_idx};
    }

    /// Specialize a generic instance and all functions it contains
    /// * create a specialized copy of the instance in module()
    /// * create specialized copies of all instance functions in module()
    /// * refer to original symbols (no new symbols are created)
    /// \param symptr       SymbolPointer to the generic instance
    /// \param cls_fn_idx   Index in class of the called method, to help resolving instance types
    /// \param loc          SourceLocation of an expression that is being compiled
    /// \returns Index of the specialized instance in module()
    ///          or no_index if the original instance is not generic
    Index specialize_instance(SymbolPointer symptr,
                              Index cls_fn_idx,
                              const SourceLocation& loc)
    {
        auto* inst_mod = symptr.symtab()->module();
        auto& inst = inst_mod->get_instance(symptr->index());
        if (!inst.is_generic())
            return no_index;

        // Resolve instance types using the m_call_args
        // and the called method (instance function with known Index)
        const auto& called_inst_fn = inst.get_function(cls_fn_idx).symptr.get_function(m_scope);
        auto resolved_types = resolve_instance_types(called_inst_fn.signature());
        auto inst_types = inst.types();
        for (auto& it : inst_types)
            resolve_generic_type(it, resolved_types);

        // Check already created specializations if one of them matches
        for (auto spec_idx : module().get_spec_instances(symptr)) {
            auto& spec_inst = module().get_instance(spec_idx);
            if (spec_inst.types() == inst_types)
                return spec_idx;
        }

        Instance spec(inst.class_(), inst.symtab());
        spec.set_types(inst_types);

        for (Index i = 0; i != inst.num_functions(); ++i) {
            auto fn_info = inst.get_function(i);
            auto specialized = specialize_function(fn_info.symptr, loc, {});
            if (specialized) {
                spec.set_function(i, specialized.scope_index, fn_info.symptr);
            } else {
                spec.set_function(i, fn_info.scope_index, fn_info.symptr);
            }
        }

        // add to specialized instance in this module
        auto spec_idx = module().add_instance(std::move(spec)).index;
        module().add_spec_instance(symptr, spec_idx);
        return spec_idx;
    }

    bool match_type_args(const TypeArgs& explicit_type_args, const TypeArgs& scope_type_args)
    {
        for (const auto& [var, ti] : explicit_type_args) {
            const TypeInfo scope_ti = scope_type_args.get(var);
            if (scope_ti != ti)  // FIXME: this needs exact comparison
                return false;
        }
        return true;
    }

    /// \returns total MatchScore of all parameters and return value, or mismatch
    /// Partial match is possible when the signature has less parameters than call args.
    MatchScore match_signature(const Signature& signature) const
    {
        Signature sig = signature;  // a copy to work on (modified below)
        MatchScore res;
        for (const auto& arg : m_call_sig.args) {
            // check there are more params to consume
            while (sig.params.empty()) {
                if (sig.return_type.type() == Type::Function) {
                    // collapse returned function, start consuming its params
                    sig = sig.return_type.signature();
                } else {
                    // unexpected argument
                    return MatchScore::mismatch();
                }
            }
            // check type of next param
            auto m = match_type(arg.type_info, sig.params[0]);
            if (!m || (!arg.literal_value && m.is_coerce()))
                return MatchScore::mismatch();
            res += m;
            // consume next param
            sig.params.erase(sig.params.begin());
        }
        if (sig.params.empty()) {
            // increase score for full match - whole signature matches the call args
            res.add_exact();
        }
        // check return type
        if (m_call_sig.return_type) {
            auto m = match_type(m_call_sig.return_type, sig.return_type);
            if (!m || m.is_coerce())
                return MatchScore::mismatch();
            res += m;
        }
        if (m_cast_type) {
            // increase score if casting target type matches return type,
            // but don't fail if it doesn't match
            auto m = match_type(m_cast_type, sig.return_type);
            if (m)
                res += m;
        }
        return res;
    }

    // Match call args with signature (which contains type vars T, U...)
    // Throw if unmatched, return resolved types for T, U... if matched
    // The result types are in the same order as the matched type vars in signature,
    // e.g. for `class MyClass T U V { my V U -> T }` it will return actual types [T, U, V].
    TypeArgs resolve_instance_types(const Signature& signature) const
    {
        const auto* sig = &signature;
        size_t i_arg = 0;
        size_t i_prm = 0;
        TypeArgs res;
        // resolve args
        for (const auto& arg : m_call_sig.args) {
            i_arg += 1;
            // check there are more params to consume
            while (i_prm >= sig->params.size()) {
                if (sig->return_type.type() == Type::Function) {
                    // collapse returned function, start consuming its params
                    sig = &sig->return_type.signature();
                    i_prm = 0;
                } else {
                    // unexpected argument
                    throw UnexpectedArgument(i_arg, TypeInfo{std::make_shared<Signature>(signature)}, arg.source_loc);
                }
            }
            // resolve T (only from original signature)
            const auto& prm = sig->params[i_prm];

            // check type of next param
            const auto m = match_type(arg.type_info, prm);
            if (!(m.is_exact() || m.is_generic() || (m.is_coerce() && arg.literal_value))) {
                throw UnexpectedArgumentType(i_arg, prm,
                        arg.type_info, arg.source_loc);
            }

            auto arg_type = arg.type_info.effective_type();
            specialize_arg(prm, arg_type, res,
                    [i_arg, &arg](const TypeInfo& exp, const TypeInfo& got) {
                        throw UnexpectedArgumentType(i_arg, exp, got,
                            arg.source_loc);
                    });

            // consume next param
            ++i_prm;
        }
        // use m_call_ret only as a hint - if return type var is still unknown
        if (signature.return_type.is_unknown()) {
            auto var = signature.return_type.generic_var();
            assert(var);
            if (!m_call_sig.return_type.is_unknown())
                res.set(var, m_call_sig.return_type);
            if (!m_cast_type.is_unknown())
                res.set(var, m_cast_type.effective_type());
            if (m_type_info)
                res.set(var, m_type_info);
        }
        return res;
    }

    Scope& m_scope;

    TypeInfo m_type_info;   // resolved ast::Type
    TypeInfo m_value_type;  // inferred type of the value
    TypeInfo m_cast_type;   // target type of Cast
    bool m_literal_value = true;  // the m_value_type is a literal (for Call args, set false if not)

    // signature for resolving overloaded functions and templates
    CallSignature m_call_sig;   // actual argument types + expected return type
};


void resolve_spec(Scope& scope, const ast::Block& block)
{
    ResolveSpecVisitor visitor {scope};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
    auto& fn = scope.function();
    if (fn.has_any_generic()) {
        // the resolved function is generic - not allowed in main scope
        if (scope.parent() == nullptr) {
            stringstream sig_str;
            sig_str << fn.name() << ':' << fn.signature();
            throw UnexpectedGenericFunction(sig_str.str());
        }
        return;
    }
    // not generic -> compile
    fn.set_compile();
}


} // namespace xci::script
