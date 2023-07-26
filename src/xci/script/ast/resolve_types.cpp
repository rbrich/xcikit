// resolve_types.cpp created on 2019-06-13 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "resolve_types.h"
#include <xci/script/typing/TypeChecker.h>
#include <xci/script/typing/overload_resolver.h>
#include <xci/script/typing/generic_resolver.h>
#include <xci/script/Value.h>
#include <xci/script/Builtin.h>
#include <xci/script/Function.h>
#include <xci/script/Error.h>
#include <xci/script/dump.h>
#include <xci/compat/macros.h>

#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/reverse.hpp>

#include <sstream>
#include <optional>

namespace xci::script {

using std::stringstream;
using ranges::views::enumerate;
using ranges::cpp20::views::reverse;


class ResolveTypesVisitor final: public ast::VisitorExclTypes {
public:
    using VisitorExclTypes::visit;

    explicit ResolveTypesVisitor(Scope& scope) : m_scope(scope) {}

    void visit(ast::Definition& dfn) override {
        // Expression might use the specified type from `dfn.symbol().get_function(m_scope).signature()`
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
        TypeChecker type_check(TypeInfo(v.ti), std::move(m_cast_type));
        const auto& spec = type_check.eval_type();  // specified/cast type
        TypeInfo::Subtypes cast_items = spec.is_struct_or_tuple() ? spec.struct_or_tuple_subtypes() : TypeInfo::Subtypes{};
        // build TypeInfo from subtypes
        std::vector<TypeInfo> subtypes;
        subtypes.reserve(v.items.size());
        for (auto&& [i, item] : v.items | enumerate) {
            m_cast_type = !cast_items.empty() ? cast_items[i] : TypeInfo{};
            item->apply(*this);
            subtypes.push_back(m_value_type.effective_type());
        }
        m_value_type = type_check.resolve(TypeInfo(std::move(subtypes)), v.source_loc);
        v.ti = m_value_type;
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
        if (m_value_type.elem_type().has_unknown() && !m_value_type.has_generic())
            throw MissingExplicitType(v.source_loc);
        v.ti = m_value_type;
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
    }

    void visit(ast::Reference& v) override {
        assert(v.identifier.symbol);
        const auto& symtab = *v.identifier.symbol.symtab();
        const auto& sym = *v.identifier.symbol;

        switch (sym.type()) {
            case Symbol::Instruction: {
                assert(m_call_sig.size() <= 1);
                CallArg arg;
                if (!m_call_sig.empty())
                    arg = std::move(m_call_sig.back().arg);
                else
                    arg.type_info = ti_void();
                // the instructions are low-level, untyped - set return type to Unknown
                m_value_type = {};
                // check type of args (they must be Int, TypeIndex or Byte)
                auto check_type = [](const TypeInfo& ti) -> bool {
                    const Type t = ti.type();
                    return t == Type::Unknown || t == Type::Byte || t == Type::Int32 || t == Type::TypeIndex;
                };
                // check args - their number and type depends on Opcode
                auto opcode = (Opcode) sym.index();
                if (opcode <= Opcode::NoArgLast) {
                    if (!arg.type_info.is_void())
                        throw UnexpectedArgumentType(ti_void(), arg.type_info, v.source_loc);
                } else if (opcode <= Opcode::L1ArgLast) {
                    if (!check_type(arg.type_info))
                        throw UnexpectedArgumentType(ti_int32(), arg.type_info, arg.source_loc);
                } else {
                    assert(opcode <= Opcode::L2ArgLast);
                    if (!arg.type_info.is_tuple() || arg.type_info.subtypes().size() != 2)
                        throw UnexpectedArgumentType(ti_tuple(ti_int32(), ti_int32()),
                                                     arg.type_info, v.source_loc);
                    for (const auto& [i, ti] : arg.type_info.subtypes() | enumerate)
                        if (!check_type(ti))
                            throw UnexpectedArgumentType(ti_int32(), ti,
                                                         ti_tuple(ti_int32(), ti_int32()), arg.type_info,
                                                         arg.source_loc);
                }
                // cleanup - args are now fully processed
                m_call_sig.clear();
                break;
            }
            case Symbol::TypeIndex: {
                if (v.ti.is_unknown()) {
                    // try to resolve via known type args
                    auto var = v.ti.generic_var();
                    const auto& type_args = m_scope.type_args();
                    auto resolved = type_args.get(var);
                    if (resolved) {
                        v.ti = resolved;
                    }
                }
                m_value_type = ti_type_index();
                m_value_type.set_literal(false);
                return;  // do not overwrite v.ti below
            }
            case Symbol::Class:
            case Symbol::Instance:
                assert(!"not implemented");
                return;
            case Symbol::Method: {
                if (v.definition) {
                    const Function& fn = v.definition->symbol().get_function(m_scope);
                    assert(m_call_sig.empty());
                    m_call_sig.emplace_back().load_from(fn.signature(), v.source_loc);
                }

                // find instance using resolved T
                std::vector<Candidate> candidates;
                Index cls_fn_idx = no_index;
                TypeInfo cls_fn_ti;
                TypeArgs inst_type_args;
                std::vector<TypeInfo> resolved_types;
                for (auto psym : v.sym_list) {
                    auto* inst_mod = psym.symtab()->module();
                    if (inst_mod == nullptr)
                        inst_mod = &module();

                    if (psym->type() == Symbol::Method) {
                        // find prototype of the function, resolve actual type of T
                        auto& cls = inst_mod->get_class(psym->index());
                        cls_fn_idx = cls.get_index_of_function(psym->ref()->index());
                        const auto& cls_fn = psym->ref().get_generic_scope().function();
                        inst_type_args = resolve_instance_types(cls_fn.signature(), m_call_sig, m_cast_type);
                        resolved_types.clear();
                        for (Index i = 1; i <= cls.symtab().count(Symbol::TypeVar); ++i) {
                            auto var_psym = cls.symtab().find_by_index(Symbol::TypeVar, i);
                            resolved_types.push_back(get_type_arg(var_psym, inst_type_args));
                        }
                        cls_fn_ti = TypeInfo{cls_fn.signature_ptr()};
                        continue;
                    }

                    assert(psym->type() == Symbol::Instance);
                    auto& inst = inst_mod->get_instance(psym->index());
                    auto inst_fn_info = inst.get_function(cls_fn_idx);
                    const auto& fn = inst_mod->get_scope(inst_fn_info.scope_index).function();
                    const auto m = match_params(inst.types(), resolved_types);
                    candidates.push_back({inst_mod, inst_fn_info.scope_index, psym, TypeInfo{fn.signature_ptr()}, cls_fn_ti, inst_type_args, m});
                }

                auto [found, conflict] = find_best_candidate(candidates);

                if (found && !conflict) {
                    v.module = found->module;
                    v.index = found->scope_index;
                    m_value_type = found->type;
                    break;
                }

                // Partial instantiation with generic args -> just resolve the type, not the concrete instance
                if (conflict && found->match.is_generic()) {
                    m_value_type = found->gen_type;
                    resolve_generic_type(m_value_type, found->type_args);
                    break;
                }

                // ERROR couldn't find single matching instance for `args`
                stringstream o_candidates;
                for (const auto& c : candidates) {
                    o_candidates << "   " << c.match << "  "
                                 << c.type.signature() << std::endl;
                }
                stringstream o_ftype;
                o_ftype << v.identifier.name;
                if (!m_call_sig.empty())
                    o_ftype << ' ' << m_call_sig.back().signature();
                if (conflict)
                    throw FunctionConflict(o_ftype.str(), o_candidates.str(), v.source_loc);
                else
                    throw FunctionNotFound(o_ftype.str(), o_candidates.str(), v.source_loc);
            }
            case Symbol::Module:
                if (sym.index() == no_index) {
                    assert(m_call_sig.size() <= 1);
                    CallArg arg;
                    if (!m_call_sig.empty())
                        arg = std::move(m_call_sig.back().arg);
                    else
                        arg.type_info = ti_void();
                    // builtin __module
                    if (!arg.type_info.is_void() && arg.type_info.type() != Type::Int32) {
                        // the arg must be Int32 (index of imported module)
                        throw UnexpectedArgumentType(ti_int32(), arg.type_info, arg.source_loc);
                    }
                    // cleanup - args are now fully processed
                    m_call_sig.clear();
                    m_value_type = ti_module();
                    break;
                }
                [[fallthrough]];
            case Symbol::Function:
            case Symbol::StructItem: {
                // specified type in declaration
                if (sym.type() == Symbol::Function && v.definition && v.ti) {
                    assert(m_call_sig.empty());
                    if (v.ti.is_callable()) {
                        m_call_sig.emplace_back().load_from(v.ti.signature(), v.source_loc);
                    } else {
                        // A naked type, consider it a function return type
                        m_call_sig.emplace_back().set_return_type(v.ti);
                    }
                }

                // Resolve overload
                auto res = resolve_overload(v.sym_list, v.identifier, v.type_args_ti);
                // The referenced function must have been defined
                if (!res.type.effective_type())
                    throw MissingExplicitType(v.identifier.name, v.identifier.source_loc);

                v.identifier.symbol = res.symptr;
                if (res.symptr->type() == Symbol::Function) {
                    v.module = res.module;
                    v.index = res.scope_index;
                    m_value_type = res.type;
                    if (v.definition) {
                        m_call_sig.clear();
                    }
                } else if (res.symptr->type() == Symbol::StructItem) {
                    // remember full function (Struct -> Item) in v.ti
                    v.ti = res.type;
                    m_value_type = res.type.signature().return_type;
                    m_value_type.set_literal(false);
                    m_call_sig.clear();
                    return;  // do not overwrite v.ti below
                } else {
                    assert(res.symptr->type() == Symbol::Module);
                    m_value_type = res.type;
                }
                break;
            }
            case Symbol::Parameter: {
                const auto* ref_scope = m_scope.find_parent_scope(&symtab);
                const auto& sig_type = ref_scope->function().parameter(sym.index());
                m_value_type = sig_type;
                break;
            }
            case Symbol::Value:
                if (sym.index() == no_index) {
                    assert(m_call_sig.size() == 1);
                    // Intrinsics
                    // __value - expects a single Int32 parameter
                    CallArg arg;
                    if (!m_call_sig.empty())
                        arg = std::move(m_call_sig.back().arg);
                    else
                        arg.type_info = ti_void();
                    if (arg.type_info.type() != Type::Int32) {
                        // the arg must be Int32 (index of imported module)
                        throw UnexpectedArgumentType(ti_int32(), arg.type_info, arg.source_loc);
                    }
                    // cleanup - args are now fully processed
                    m_call_sig.clear();
                    // __value returns index (Int32)
                    m_value_type = ti_int32();
                } else {
                    m_value_type = v.ti;
                }
                break;
            case Symbol::TypeName:
            case Symbol::TypeVar:
                assert(!"not implemented");
                return;
            case Symbol::Nonlocal:
            case Symbol::Unresolved:
                XCI_UNREACHABLE;
        }
        m_value_type.set_literal(false);
        v.ti = m_value_type;
    }

    void visit(ast::Call& v) override {
        if (v.definition) {
            Function& fn = v.definition->symbol().get_function(m_scope);
            if (!fn.signature().has_nonvoid_param())
                m_type_info = fn.signature().return_type;
            else
                m_type_info = TypeInfo{fn.signature_ptr()};
        }

        TypeChecker type_check(std::move(m_type_info), std::move(m_cast_type));

        // resolve each argument
        CallArg call_arg;
        auto orig_call_sig = std::move(m_call_sig);
        if (v.arg) {
            m_call_sig.clear();
            v.arg->apply(*this);
            assert(v.arg->source_loc);
            call_arg = {m_value_type.effective_type(), v.arg->source_loc};
        } else {
            call_arg.type_info = ti_void();
        }
        // move args to m_call_args (note that m_call_args might be used
        // when evaluating each argument, so we cannot push to them above)
        m_call_sig = std::move(orig_call_sig);
        m_call_sig.emplace_back().set_arg(std::move(call_arg));
        m_call_sig.back().set_return_type(std::move(type_check.eval_type()));

        // using resolved args, resolve the callable itself
        // (it may use args types for overload resolution)
        v.callable->apply(*this);

        if (!m_value_type.is_callable() && !m_value_type.is_unknown() && !m_call_sig.empty()) {
            throw UnexpectedArgument(m_value_type, m_call_sig.back().arg.source_loc);
        }

        if (m_value_type.is_callable()) {
            // result is new signature with args removed (applied)
            const auto param_type_args = resolve_generic_args_to_signature(m_value_type.signature(), m_call_sig);
            store_resolved_param_type_vars(m_scope, param_type_args);
            auto return_type = resolve_return_type_from_call_args(m_value_type.signature_ptr(), v);
            if (v.definition == nullptr) {
                // all args consumed, or a zero-arg function being called
                // -> effective type is the return type
                m_value_type = std::move(return_type);
            } else {
                // Not really calling, just defining, e.g. `f = compose u v`
                // Keep the return type as is, making it `() -> <lambda type>`
                auto sig = std::make_shared<Signature>();
                sig->set_parameter(ti_void());
                sig->set_return_type(std::move(return_type));
                m_value_type = TypeInfo{sig};
            }
        } else if (m_value_type.is_unknown()) {
            // if the callable has generic type F, we cannot process it now - reset to Unknown
            m_value_type = {};
        }

        m_call_sig.clear();
        v.ti = m_value_type;
    }

    void visit(ast::OpCall& v) override {
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        TypeInfo expr_type;
        bool all_literal = true;
        for (auto& item : v.if_then_expr) {
            item.first->apply(*this);
            if (m_value_type != ti_bool())
                throw ConditionNotBool();
            item.second->apply(*this);
            all_literal = all_literal && m_value_type.is_literal();
            // check that all then-expressions have the same type
            if (&item == &v.if_then_expr.front()) {
                expr_type = m_value_type;
            } else {
                if (expr_type != m_value_type)
                    throw BranchTypeMismatch(expr_type, m_value_type);
            }
        }

        v.else_expr->apply(*this);
        if (expr_type != m_value_type)
            throw BranchTypeMismatch(expr_type, m_value_type);

        m_value_type.set_literal(all_literal && m_value_type.is_literal());
    }

    void visit(ast::WithContext& v) override {
        // resolve type of context (StructInit leads to incomplete struct type)
        v.context->apply(*this);
        // lookup the enter function with the resolved context type
        assert(m_call_sig.empty());
        m_call_sig.emplace_back().set_arg({m_value_type, v.context->source_loc});
        m_call_sig.back().set_return_type(ti_unknown());
        v.enter_function.apply(*this);
        m_call_sig.clear();
        assert(m_value_type.is_callable());
        auto enter_sig = m_value_type.signature();
        // re-resolve type of context (match actual struct type as found by resolving `with` function)
        m_cast_type = enter_sig.param_type;
        v.context->apply(*this);
        m_cast_type = {};
        assert(m_value_type == enter_sig.param_type);
        // lookup the leave function, it's arg type is same as enter functions return type
        v.leave_type = enter_sig.return_type.effective_type();
        m_call_sig.emplace_back().set_arg({v.leave_type, v.context->source_loc});
        m_call_sig.back().set_return_type(ti_void());
        v.leave_function.apply(*this);
        m_call_sig.clear();
        // resolve type of expression - it's also the type of the whole "with" expression
        v.expression->apply(*this);
        m_value_type.set_literal(false);
    }

    void visit(ast::Function& v) override {
        if (v.symbol->type() == Symbol::Function) {
            v.scope_index = v.symbol.get_scope_index(m_scope);
        }
        auto& scope = module().get_scope(v.scope_index);

        Function& fn = scope.function();

        m_value_type = TypeInfo{fn.signature_ptr()};
        v.call_arg = !m_call_sig.empty();

        if (fn.has_generic_param()) {
            resolve_types(scope, v.body);
            m_value_type = TypeInfo{fn.signature_ptr()};
        } else {
            // compile body and resolve return type
            if (v.definition) {
                // in case the function is recursive, propagate the type upwards
                auto symptr = v.definition->symbol();
                auto& fn_dfn = symptr.get_function(m_scope);
                fn_dfn.set_signature(m_value_type.signature_ptr());
            }
            resolve_types(scope, v.body);
            m_value_type = TypeInfo{fn.signature_ptr()};
        }

        // parameterless function is equivalent to its return type (eager evaluation)
        /* while (m_value_type.is_callable() && m_value_type.signature().params.empty()) {
            m_value_type = m_value_type.signature().return_type;
        }*/

        m_value_type.set_literal(false);
        v.ti = m_value_type;
    }

    // The cast expression is translated to a call to `cast` method from the Cast class.
    // The inner expression type and the cast type are used to look up the instance of Cast.
    void visit(ast::Cast& v) override {
        // resolve the inner expression -> m_value_type
        // (the Expression might use the specified type from `m_cast_type`)
        resolve_generic_type(v.to_type, m_scope);
        m_cast_type = v.to_type;
        m_call_sig.clear();
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
        m_call_sig.emplace_back().set_arg({m_value_type, v.expression->source_loc});
        m_call_sig.back().set_return_type(v.to_type);
        v.cast_function->apply(*this);
        // set the effective type of the Cast expression and clean the call types
        m_value_type = std::move(m_call_sig.back().return_type);
        m_value_type.set_literal(false);
        m_call_sig.clear();
    }

private:
    Module& module() const { return m_scope.module(); }
    Function& function() const { return m_scope.function(); }

    // Check return type matches and set it to concrete type if it's generic.
    void resolve_return_type(Signature& sig, const TypeInfo& deduced,
                             Scope& scope, const SourceLocation& loc) const
    {
        if (sig.return_type.has_unknown()) {
            if (deduced.is_unknown() && !deduced.has_generic()) {
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
            sig.set_return_type(deduced);  // Unknown/var=0 not handled by resolve_type_vars
            return;
        }
        if (sig.return_type != deduced)
            throw UnexpectedReturnType(sig.return_type, deduced);
    }

    /// Find matching function overload according to m_call_args
    Candidate resolve_overload(const SymbolPointerList& sym_list, const ast::Identifier& identifier,
                               const std::vector<TypeInfo>& type_args)
    {
        std::vector<Candidate> candidates;
        for (auto symptr : sym_list) {
            // resolve nonlocal
            while (symptr->depth() != 0)
                symptr = symptr->ref();

            auto* symmod = symptr.symtab()->module();
            assert(symmod != nullptr);
            Index scope_idx;
            SignaturePtr sig_ptr;
            TypeArgs res_type_args;
            if (symptr->type() == Symbol::Function) {
                scope_idx = symptr.get_generic_scope_index();
                auto& fn = symmod->get_scope(scope_idx).function();
                if (type_args.size() > fn.num_type_params()) {
                    // skip - not enough type vars for explicit type args
                    candidates.push_back({symmod, scope_idx, symptr, TypeInfo{fn.signature_ptr()}, {}, {}, MatchScore{-1}});
                    continue;
                }
                if (!type_args.empty()) {
                    res_type_args = symmod->get_scope(scope_idx).type_args();
                    unsigned i = 0;
                    bool compatible = true;
                    for (auto var : fn.symtab().filter(Symbol::TypeVar)) {
                        if (var->name().front() == '$')
                            continue;
                        set_type_arg(var, type_args[i], res_type_args,
                                     [&compatible](const TypeInfo& exp, const TypeInfo& got)
                                     { compatible = false; });
                        if (++i >= type_args.size())
                            break;
                    }
                    if (!compatible) {
                        // skip - incompatible type args
                        candidates.push_back({symmod, scope_idx, symptr, TypeInfo{fn.signature_ptr()}, {}, std::move(res_type_args), MatchScore{-1}});
                        continue;
                    }
                    sig_ptr = std::make_shared<Signature>(fn.signature());  // copy
                    resolve_type_vars(*sig_ptr, res_type_args);
                } else {
                    sig_ptr = fn.signature_ptr();
                }
            } else if (symptr->type() == Symbol::StructItem) {
                // Special symbol that says the name may be a struct member.
                // In that case the arg has to be a struct with the referenced member name.
                // If it isn't, skip the candidate.
                if (!m_call_sig.empty() && m_call_sig.back().arg.type_info.is_struct()) {
                    const auto& struct_type = m_call_sig.back().arg.type_info;
                    const auto* item_type = struct_type.struct_item_by_name(symptr->name());
                    if (item_type == nullptr) {
                        // skip - struct doesn't have the referenced member
                        continue;
                    }
                    scope_idx = no_index;
                    sig_ptr = std::make_shared<Signature>();
                    sig_ptr->set_parameter(TypeInfo{struct_type});
                    sig_ptr->set_return_type(*item_type);
                } else {
                    // skip - not a struct
                    continue;
                }
            } else {
                assert(symptr->type() == Symbol::Module);
                scope_idx = no_index;
                Module& imp_mod = symptr.get_module();
                sig_ptr = imp_mod.get_main_function().signature_ptr();
            }
            auto match = match_signature(*sig_ptr, m_call_sig, m_cast_type);
            candidates.push_back({symmod, scope_idx, symptr, TypeInfo{std::move(sig_ptr)}, {}, std::move(res_type_args), match});
        }

        auto [found, conflict] = find_best_candidate(candidates);

        if (found && !conflict) {
            if (found->type.has_generic()) {
                auto call_type_args = specialize_signature(found->type.signature_ptr(), m_call_sig, found->type_args);
                if (!call_type_args.empty()) {
                    // resolve generic vars to received types
                    auto new_sig = std::make_shared<Signature>(found->type.signature());  // copy
                    resolve_type_vars(*new_sig, call_type_args);
                    return Candidate {
                        .module = found->module,
                        .scope_index = found->scope_index,
                        .symptr = found->symptr,
                        .type = TypeInfo(std::move(new_sig)),
                    };
                }
            }
            return *found;
        }

        // format the error message (candidates)
        stringstream o_candidates;
        for (const auto& c : candidates) {
            o_candidates << "   " << c.match << "  ";
            if (!c.type_args.empty())
                o_candidates << '<' << c.type_args << "> ";
            o_candidates << c.type.signature() << std::endl;
        }
        stringstream o_ftype;
        o_ftype << identifier.name;
        if (!type_args.empty()) {
            o_ftype << '<';
            for (const auto& type_arg : type_args) {
                if (&type_arg != &type_args.front())
                    o_ftype << ", ";
                o_ftype << type_arg;
            }
            o_ftype << '>';
        }
        if (!m_call_sig.empty())
            o_ftype << ' ' << m_call_sig.back().signature();
        if (conflict) {
            // ERROR found multiple matching functions
            throw FunctionConflict(o_ftype.str(), o_candidates.str(), identifier.source_loc);
        } else {
            // special case for StructItem, which is invisible in candidates - report as if the symbol wasn't resolved at all
            if (sym_list.size() == 1 && sym_list.front()->type() == Symbol::StructItem)
                throw UndefinedName(identifier.name, identifier.source_loc);
            // ERROR couldn't find matching function for `args`
            throw FunctionNotFound(o_ftype.str(), o_candidates.str(), identifier.source_loc);
        }
    }

    /// Resolve return type after applying m_call_sig
    // FIXME: share with resolve_spec
    TypeInfo resolve_return_type_from_call_args(const SignaturePtr& signature, ast::Call& v)
    {
        SignaturePtr sig;
        const TypeArgs call_type_args = specialize_signature(signature, m_call_sig);
        v.wrapped_execs = 0;
        for (const CallSignature& call_sig : m_call_sig | reverse) {
            if (!sig)
                sig = std::make_shared<Signature>(*signature);
            // check there are more params to consume
            else if (sig->return_type.type() == Type::Function) {
                // collapse returned function, start consuming its params
                sig = std::make_shared<Signature>(sig->return_type.signature());
                ++v.wrapped_execs;
            } else {
                // checked by specialize_signature() above
                assert(!"unexpected return type");
            }
            // skip blocks / functions without params
            while (sig->param_type.is_void() && sig->return_type.type() == Type::Function) {
                sig = sig->return_type.signature_ptr();
                ++v.wrapped_execs;
            };
            const auto& c_sig = call_sig.signature();
            const auto& source_loc = call_sig.arg.source_loc;
            {
                // check type of next param
                const auto& sig_type = sig->param_type;
                const auto& call_type = c_sig.param_type;
                const auto m = match_type(call_type, sig_type);
                if (!m)
                    throw UnexpectedArgumentType(sig_type, sig_type, source_loc);
                if (m.is_coerce()) {
                    // Update type_info of the coerced literal argument
                    m_cast_type = sig_type;
                    auto orig_call_sig = std::move(m_call_sig);
                    m_call_sig.clear();
                    v.arg->apply(*this);
                    m_call_sig = std::move(orig_call_sig);
                    m_cast_type = {};
                }
                // FIXME: move into second pass outside resolve_return_type_from_call_args()
                if (sig_type.is_callable()) {
                    // resolve overload in case the arg is a function that was specialized
                    auto orig_call_sig = std::move(m_call_sig);
                    m_call_sig.clear();
                    m_call_sig.emplace_back().load_from(sig_type.signature(), source_loc);
                    v.arg->apply(*this);
                    m_call_sig = std::move(orig_call_sig);
                }
                if (sig_type.is_struct_or_tuple() && !sig_type.is_void()) {
                    // resolve overload in case the arg tuple contains a function that was specialized
                    auto* tuple = dynamic_cast<ast::Tuple*>(v.arg.get());
                    if (tuple && !tuple->items.empty()) {
                        auto sig_subtypes = sig_type.struct_or_tuple_subtypes();
                        assert(tuple->items.size() == sig_subtypes.size());
                        auto orig_call_sig = std::move(m_call_sig);
                        for (auto&& [i, sig_item] : sig_subtypes | enumerate) {
                            if (sig_item.is_callable()) {
                                m_call_sig.clear();
                                m_call_sig.emplace_back().load_from(sig_item.signature(), source_loc);
                                tuple->items[i]->apply(*this);
                            }
                        }
                        m_call_sig = std::move(orig_call_sig);
                    }
                }
            }
        }
        auto res = sig->return_type;
        resolve_generic_type(res, call_type_args);
        return res;
    }

    Scope& m_scope;

    TypeInfo m_type_info;   // resolved ast::Type
    TypeInfo m_value_type;  // inferred type of the value
    TypeInfo m_cast_type;   // target type of Cast

    // signature for resolving overloaded functions and templates
    std::vector<CallSignature> m_call_sig;   // actual argument types + expected return type
};


void resolve_types(Scope& scope, const ast::Block& block)
{
    ResolveTypesVisitor visitor {scope};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
    auto& fn = scope.function();
    if (!fn.has_any_generic() && !scope.has_unresolved_type_params()) {
        // not generic -> compile
        fn.set_compile();
    }
}


} // namespace xci::script
