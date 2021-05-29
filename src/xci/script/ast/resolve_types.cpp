// resolve_types.cpp created on 2019-06-13 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "resolve_types.h"
#include <xci/script/Value.h>
#include <xci/script/Builtin.h>
#include <xci/script/Function.h>
#include <xci/script/Error.h>
#include <xci/compat/macros.h>

#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/algorithm.hpp>

#include <sstream>
#include <optional>
#include <unordered_set>

namespace xci::script {

using std::move;
using std::make_unique;
using std::stringstream;
using std::endl;

using ranges::views::enumerate;
using ranges::views::filter;
using ranges::any_of;
using ranges::to;


enum class Match { None, Generic, Partial, Exact };

static Match match_type(const TypeInfo& inferred, const TypeInfo& actual);
static Match match_struct(const TypeInfo& inferred, const TypeInfo& actual);


class TypeCheckHelper {
public:
    explicit TypeCheckHelper(TypeInfo&& spec) : m_specified_type(move(spec)) {}
    TypeCheckHelper(TypeInfo&& spec, TypeInfo&& cast) : m_specified_type(move(spec)) {
        if (cast)
            m_specified_type = move(cast);
    }

    void check(const TypeInfo& inferred, const SourceLocation& loc) const {
        const auto& specified = type();
        if (!specified)
            return;
        if (specified.is_struct() && inferred.is_struct()) {
            if (match_struct(inferred, specified) == Match::None)
                throw StructTypeMismatch(specified, loc);
            return;
        }
        if (inferred != specified)
            throw DefinitionTypeMismatch(specified, inferred, loc);
    }

    void check_struct_item(const std::string& key, const TypeInfo& inferred, const SourceLocation& loc) const {
        assert(type().is_struct());
        const auto& spec_items = type().struct_items();
        auto spec_it = std::find_if(spec_items.begin(), spec_items.end(),
            [&key](const TypeInfo::StructItem& spec) {
              return spec.first == key;
            });
        if (spec_it == spec_items.end())
            throw StructUnknownKey(type(), key, loc);
        if (match_type(inferred, spec_it->second) == Match::None)
            throw StructKeyTypeMismatch(type(), spec_it->second, inferred, loc);
    }

    const TypeInfo& type() const { return m_specified_type; }
    TypeInfo&& type() { return move(m_specified_type); }

private:
    TypeInfo m_specified_type;
};


/// \returns Match: None/Generic/Partial
static Match match_type(const TypeInfo& inferred, const TypeInfo& actual)
{
    if (actual.is_struct() && inferred.is_struct())
        return match_struct(inferred, actual);
    return inferred == actual
        ? ( (actual.is_generic() || inferred.is_generic()) ? Match::Generic : Match::Partial)
        : Match::None;
}


/// Match incomplete Struct type from ast::StructInit to resolved Struct type.
/// All keys and types from inferred are checked against resolved.
/// Partial match is possible when inferred has less keys than resolved.
/// \param inferred     Possibly incomplete Struct type as constructed from AST
/// \param resolved     Actual resolved type for the value
/// \returns  Match: None/Generic/Partial (full match not distinguished)
static Match match_struct(const TypeInfo& inferred, const TypeInfo& actual)
{
    assert(inferred.is_struct());
    assert(actual.is_struct());
    const auto& actual_items = actual.struct_items();
    auto res = Match::Partial;
    for (const auto& inf : inferred.struct_items()) {
        auto act_it = std::find_if(actual_items.begin(), actual_items.end(),
                [&inf](const TypeInfo::StructItem& act) {
                  return act.first == inf.first;
                });
        if (act_it == actual_items.end())
            return Match::None;  // not found
        // check item type
        auto m = match_type(TypeInfo(inf.second), act_it->second);
        switch (m) {
            case Match::None: return m;  // item type doesn't match
            case Match::Generic: res = m; break; // unknown/generic
            default: break;
        }
    }
    return res;
}


// Check return type matches and set it to concrete type if it's generic.
static void resolve_return_type(Signature& sig, const TypeInfo& t, const SourceLocation& loc)
{
    if (!sig.return_type) {
        if (!t && !sig.is_generic())
            throw MissingExplicitType(loc);
        sig.return_type = t;
        return;
    }
    if (sig.return_type != t)
        throw UnexpectedReturnType(sig.return_type, t);
}


class TypeCheckerVisitor final: public ast::Visitor {
    struct CallArg {
        TypeInfo type_info;
        SourceLocation source_loc;
    };
    using CallArgs = std::vector<CallArg>;

public:
    explicit TypeCheckerVisitor(Function& func, const std::vector<TypeInfo>& type_args)
        : m_function(func), m_type_args(type_args) {}

    void visit(ast::Definition& dfn) override {
        // Evaluate specified type
        if (dfn.variable.type) {
            dfn.variable.type->apply(*this);
        } else {
            m_type_info = {};
        }

        if (m_class != nullptr) {
            auto idx = m_class->add_function_type(move(m_type_info));
            dfn.variable.identifier.symbol->set_index(idx);
            return;
        }

        if (m_instance != nullptr) {
            // evaluate type according to class and type vars
            auto& psym = dfn.variable.identifier.symbol;
            TypeInfo eval_type = m_instance->class_().get_function_type(psym->ref()->index());
            for (const auto&& [i, t] : m_instance->types() | enumerate)
                eval_type.replace_var(i + 1, t);

            // specified type is basically useless here, let's just check
            // it matches evaluated type from class instance
            if (m_type_info && m_type_info != eval_type)
                throw DefinitionTypeMismatch(m_type_info, eval_type, dfn.expression->source_loc);

            m_type_info = move(eval_type);

            m_instance->set_function(psym->ref()->index(), psym->index());
        }

        // Expression might use the specified type from `m_type_info`
        if (dfn.expression)
            dfn.expression->apply(*this);

        if (m_instance != nullptr)
            return;

        Function& func = module().get_function(dfn.symbol()->index());
        if (m_value_type.is_callable())
            func.signature() = m_value_type.signature();
        else
            resolve_return_type(func.signature(), m_value_type, dfn.expression->source_loc);
        m_value_type = {};
    }

    void visit(ast::Invocation& inv) override {
        inv.expression->apply(*this);
        auto res_type = m_value_type.effective_type();
        if (!res_type.is_void())
            inv.type_id = get_type_id(move(res_type));
    }

    void visit(ast::Return& ret) override {
        ret.expression->apply(*this);
        resolve_return_type(m_function.signature(), m_value_type,
                ret.expression->source_loc);
    }

    void visit(ast::Class& v) override {
        m_class = &module().get_class(v.index);
        for (auto& dfn : v.defs)
            dfn.apply(*this);
        m_class = nullptr;
    }

    void visit(ast::Instance& v) override {
        m_instance = &module().get_instance(v.index);
        // resolve instance types
        for (auto& t : v.type_inst) {
            t->apply(*this);
            m_instance->add_type(move(m_type_info));
        }
        // resolve each Definition from the class,
        // fill-in FunctionType, match with possible named arguments and body
        for (auto& dfn : v.defs)
            dfn.apply(*this);
        m_instance = nullptr;
    }

    void visit(ast::TypeDef& v) override {
        v.type->apply(*this);

        // create new Named type
        Index index = module().add_type(TypeInfo{v.type_name.name, move(m_type_info)});
        v.type_name.symbol->set_index(index);
    }

    void visit(ast::TypeAlias& v) override {
        v.type->apply(*this);

        // add the actual type to Module, referenced by symbol
        Index index = module().add_type(move(m_type_info));
        v.type_name.symbol->set_index(index);
    }

    void visit(ast::Literal& v) override {
        m_value_type = v.value.type_info();
        TypeCheckHelper type_check(move(m_type_info));
        type_check.check(m_value_type, v.source_loc);
    }

    void visit(ast::Bracketed& v) override {
        v.expression->apply(*this);
    }

    void visit(ast::Tuple& v) override {
        TypeCheckHelper type_check(move(m_type_info));
        // build TypeInfo from subtypes
        std::vector<TypeInfo> subtypes;
        subtypes.reserve(v.items.size());
        for (auto& item : v.items) {
            item->apply(*this);
            subtypes.push_back(m_value_type.effective_type());
        }
        m_value_type = TypeInfo(move(subtypes));
        type_check.check(m_value_type, v.source_loc);
    }

    void visit(ast::List& v) override {
        TypeCheckHelper type_check(move(m_type_info));
        // check all items have same type
        TypeInfo elem_type;
        for (auto& item : v.items) {
            item->apply(*this);
            if (item.get() == v.items.front().get()) {
                // first item
                elem_type = move(m_value_type);
            } else {
                // other items
                if (elem_type != m_value_type)
                    throw ListElemTypeMismatch(elem_type, m_value_type);
            }
        }
        v.elem_type_id = get_type_id(elem_type);
        m_value_type = ti_list(move(elem_type));
        type_check.check(m_value_type, v.source_loc);
    }

    void visit(ast::StructInit& v) override {
        if (v.struct_type) {
            // second pass (from ast::WithContext):
            // * v.struct_type is the inferred type
            // * m_type_info is the final struct type
            if (m_type_info.is_unknown()) {
                m_value_type = v.struct_type;
                return;
            }
            if (m_type_info.type() != Type::Struct)
                throw StructTypeMismatch(m_type_info, v.source_loc);
            if (match_struct(v.struct_type, m_type_info) == Match::None)
                throw StructTypeMismatch(m_type_info, v.source_loc);
            v.struct_type = move(m_type_info);
            m_value_type = v.struct_type;
            return;
        }
        // first pass - resolve incomplete struct type
        //              and check it matches specified type (if any)
        TypeCheckHelper type_check(move(m_type_info), move(m_cast_type));
        const auto& specified = type_check.type();
        if (specified && specified.type() != Type::Struct)
            throw StructTypeMismatch(specified, v.source_loc);
        // build TypeInfo for the struct initializer
        TypeInfo::StructItems ti_items;
        ti_items.reserve(v.items.size());
        std::unordered_set<std::string> keys;
        for (auto& item : v.items) {
            // check the key is not duplicate
            auto [_, ok] = keys.insert(item.first.name);
            if (!ok)
                throw StructDuplicateKey(item.first.name, item.second->source_loc);
            // resolve item type
            item.second->apply(*this);
            auto item_type = m_value_type.effective_type();
            if (specified)
                type_check.check_struct_item(item.first.name, item_type, item.second->source_loc);
            ti_items.emplace_back(item.first.name, item_type);
        }
        v.struct_type = TypeInfo(move(ti_items));
        if (specified) {
            assert(match_struct(v.struct_type, specified) != Match::None);  // already checked above
            v.struct_type = std::move(type_check.type());
        }
        m_value_type = v.struct_type;
    }

    void visit(ast::Reference& v) override {
        assert(v.identifier.symbol);
        const auto& symtab = *v.identifier.symbol.symtab();
        const auto& sym = *v.identifier.symbol;

        if (v.type_arg) {
            if (sym.type() != Symbol::TypeId)
                throw UnexpectedTypeArg(v.type_arg->source_loc);
            v.type_arg->apply(*this);
        }

        switch (sym.type()) {
            case Symbol::Instruction: {
                // the instructions are low-level, untyped - set return type to Unknown
                m_value_type = {};
                m_intrinsic = true;
                // check number of args - it depends on Opcode
                auto opcode = (Opcode) sym.index();
                if (opcode <= Opcode::NoArgLast) {
                    if (!m_call_args.empty())
                        throw UnexpectedArgumentCount(0, m_call_args.size(), v.source_loc);
                } else if (opcode <= Opcode::L1ArgLast) {
                    if (m_call_args.size() != 1)
                        throw UnexpectedArgumentCount(1, m_call_args.size(), v.source_loc);
                } else {
                    assert(opcode <= Opcode::L2ArgLast);
                    if (m_call_args.size() != 2)
                        throw UnexpectedArgumentCount(2, m_call_args.size(), v.source_loc);
                }
                // check type of args (they must be Int or Byte)
                for (const auto&& [i, arg] : m_call_args | enumerate) {
                    Type t = arg.type_info.type();
                    if (t != Type::Unknown && t != Type::Byte && t != Type::Int32)
                        throw UnexpectedArgumentType(i+1, ti_int32(),
                                                     arg.type_info, arg.source_loc);
                }
                // cleanup - args are now fully processed
                m_call_args.clear();
                return;
            }
            case Symbol::TypeId: {
                if (!v.type_arg)
                    throw MissingTypeArg(v.source_loc);
                if (m_type_info.is_unknown()) {
                    // try to resolve via known type args
                    auto var = m_type_info.generic_var();
                    if (var > 0 && var <= m_type_args.size())
                        m_type_info = m_type_args[var-1];
                    else {
                        // unresolved -> unknown type id
                        m_value_type = {};
                        break;
                    }
                }
                // Record the resolved Type ID for Compiler
                v.index = get_type_id(move(m_type_info));
                m_value_type = ti_int32();
                break;
            }
            case Symbol::Class:
            case Symbol::Instance:
                // TODO
                return;
            case Symbol::Method: {
                // find prototype of the function, resolve actual type of T
                const auto& symmod = symtab.module() == nullptr ? module() : *symtab.module();
                auto& cls = symmod.get_class(sym.index());
                const auto& cls_fn = cls.get_function_type(sym.ref()->index());
                auto inst_types = resolve_instance_types(cls_fn.signature());
                // find instance using resolved T
                struct Candidate {
                    Module* module;
                    Index index;
                    Match match;
                };
                std::vector<Candidate> candidates;
                auto inst_psym = v.chain;
                while (inst_psym) {
                    assert(inst_psym->type() == Symbol::Instance);
                    auto* inst_mod = inst_psym.symtab()->module();
                    if (inst_mod == nullptr)
                        inst_mod = &module();
                    auto& inst = inst_mod->get_instance(inst_psym->index());
                    auto fn_idx = inst.get_function(sym.ref()->index());
                    if (inst.types() == inst_types) {
                        // find instance function
                        Match m = (ranges::any_of(inst_types, [](const TypeInfo& t) { return t.is_unknown(); }))
                                  ? Match::Partial : Match::Exact;
                        candidates.push_back({inst_mod, fn_idx, m});
                    } else {
                        candidates.push_back({inst_mod, fn_idx, Match::None});
                    }
                    inst_psym = inst_psym->next();
                }

                // check for single exact match
                const auto exact_candidates = candidates
                        | filter([](const Candidate& c){ return c.match == Match::Exact; })
                        | to<std::vector>();
                if (exact_candidates.size() == 1) {
                    const auto& c = exact_candidates.front();
                    auto& fn = c.module->get_function(c.index);
                    v.module = c.module;
                    v.index = c.index;
                    m_value_type = TypeInfo{fn.signature_ptr()};
                    break;
                }

                // check for single partial match
                if (exact_candidates.empty()) {
                    const auto partial_candidates = candidates
                          | filter([](const Candidate& c){ return c.match == Match::Partial; })
                          | to<std::vector>();
                    if (partial_candidates.size() == 1) {
                        const auto& c = partial_candidates.front();
                        auto& fn = c.module->get_function(c.index);
                        v.module = c.module;
                        v.index = c.index;
                        m_value_type = TypeInfo{fn.signature_ptr()};
                        break;
                    }
                }

                // ERROR couldn't find single matching instance for `args`
                stringstream o_candidates;
                for (const auto& c : candidates) {
                    auto& fn = c.module->get_function(c.index);
                    o_candidates << "   " << match_to_cstr(c.match) << "  "
                                 << fn.signature() << endl;
                }
                stringstream o_ftype;
                for (const auto& arg : m_call_args) {
                    if (&arg != &m_call_args.front())
                        o_ftype << ' ';
                    o_ftype << arg.type_info;
                }
                if (m_call_ret)
                    o_ftype << " -> " << m_call_ret;
                if (exact_candidates.empty())
                    throw FunctionNotFound(v.identifier.name, o_ftype.str(), o_candidates.str());
                else
                    throw FunctionConflict(v.identifier.name, o_ftype.str(), o_candidates.str(), v.identifier.source_loc);
            }
            case Symbol::Function: {
                // find matching instance
                auto symptr = v.identifier.symbol;
                struct Item {
                    Module* module;
                    Index index;
                    SymbolPointer symptr;
                    TypeInfo type;
                    Match match;
                };
                std::vector<Item> candidates;
                while (symptr) {
                    while (symptr->depth() != 0)
                        symptr = symptr->ref();

                    auto* symmod = symptr.symtab()->module();
                    if (symmod == nullptr)
                        symmod = &module();
                    auto& fn = symmod->get_function(symptr->index());
                    const auto& sig_ptr = fn.signature_ptr();
                    auto m = match_params(*sig_ptr);
                    candidates.push_back({symmod, symptr->index(), symptr, TypeInfo{sig_ptr}, m});

                    // Check also specializations when the function is generic
                    if (fn.detect_generic()) {
                        for (auto spec_idx : module().get_spec_functions(symptr)) {
                            auto& spec_fn = module().get_function(spec_idx);
                            const auto& spec_sig = spec_fn.signature_ptr();
                            auto spec_m = match_params(*spec_sig);
                            candidates.push_back({&module(), spec_idx, {}, TypeInfo{spec_sig}, spec_m});
                        }
                    }

                    symptr = symptr->next();
                }

                // find best match from candidates
                bool conflict = false;
                Match m = Match::None;
                Item* found = nullptr;
                for (auto& item : candidates) {
                    if (item.match == Match::None)
                        continue;
                    if (item.match > m) {
                        // found better match
                        m = item.match;
                        found = &item;
                        conflict = false;
                        continue;
                    }
                    if (item.match == m) {
                        // found same match -> conflict
                        conflict = true;
                    }
                }

                if (found && !conflict) {
                    if (found->symptr) {
                        auto specialized = specialize_function(found->symptr, v.source_loc);
                        if (specialized) {
                            v.module = &module();
                            v.index = specialized->index;
                            m_value_type = move(specialized->type_info);
                            break;
                        }
                    }
                    v.module = found->module;
                    v.index = found->index;
                    m_value_type = found->type;
                    break;
                }

                // format the error message (candidates)
                stringstream o_candidates;
                for (const auto& c : candidates) {
                    auto& fn = c.module->get_function(c.index);
                    o_candidates << "   " << match_to_cstr(c.match) << "  "
                                 << fn.signature() << endl;
                }
                stringstream o_args;
                for (const auto& arg : m_call_args) {
                    o_args << arg.type_info << ' ';
                }

                if (conflict) {
                    // ERROR found multiple matching functions
                    throw FunctionConflict(v.identifier.name, o_args.str(), o_candidates.str(), v.identifier.source_loc);
                } else {
                    // ERROR couldn't find matching function for `args`
                    throw FunctionNotFound(v.identifier.name, o_args.str(), o_candidates.str());
                }
            }
            case Symbol::Module:
                m_value_type = TypeInfo{Type::Module};
                break;
            case Symbol::Nonlocal: {
                assert(sym.ref());
                auto nl_sym = sym.ref();
                // owning function of the nonlocal symbol
                auto* nl_owner = nl_sym.symtab()->function();
                assert(nl_owner != nullptr);
                switch (nl_sym->type()) {
                    case Symbol::Parameter:
                        m_value_type = nl_owner->parameter(nl_sym->index());
                        break;
                    case Symbol::Function: {
                        auto specialized = specialize_function(nl_sym, v.source_loc);
                        if (specialized) {
                            v.module = &module();
                            v.index = specialized->index;
                            m_value_type = move(specialized->type_info);
                            break;
                        }
                        auto& fn = nl_owner->module().get_function(nl_sym->index());
                        v.module = nl_sym.symtab()->module();
                        v.index = nl_sym->index();
                        m_value_type = TypeInfo(fn.signature_ptr());
                        break;
                    }
                    default:
                        assert(!"non-local must reference a parameter or a function");
                        return;
                }
                m_function.add_nonlocal(TypeInfo{m_value_type});
                break;
            }
            case Symbol::Parameter:
                m_value_type = m_function.parameter(sym.index());
                break;
            case Symbol::Value:
                m_value_type = symtab.module()->get_value(sym.index()).type_info();
                break;
            case Symbol::TypeName:
            case Symbol::TypeVar:
                // TODO
                return;
            case Symbol::Unresolved:
                UNREACHABLE;
        }
//        if (sym.type() == Symbol::Function)
//            m_value_type = m_value_type.effective_type();
        // FIXME: remove, this writes to builtin etc.
        v.identifier.symbol->set_callable(m_value_type.is_callable());
    }

    void visit(ast::Call& v) override {
        TypeCheckHelper type_check(move(m_type_info), move(m_cast_type));

        // resolve each argument
        CallArgs args;
        for (auto& arg : v.args) {
            arg->apply(*this);
            assert(arg->source_loc);
            args.push_back({m_value_type.effective_type(), arg->source_loc});
        }
        // append args to m_call_args (note that m_call_args might be used
        // when evaluating each argument, so we cannot push to them above)
        std::move(args.begin(), args.end(), std::back_inserter(m_call_args));
        m_call_ret = move(type_check.type());
        m_intrinsic = false;

        // using resolved args, resolve the callable itself
        // (it may use args types for overload resolution)
        v.callable->apply(*this);
        v.intrinsic = m_intrinsic;

        if (m_value_type && !m_value_type.is_callable() && !m_call_args.empty()) {
            throw UnexpectedArgument(1, m_call_args[0].source_loc);
        }

        if (m_value_type.is_callable()) {
            // result is new signature with args removed (applied)
            auto new_signature = resolve_params(m_value_type.signature(), v);
            if (new_signature->params.empty()) {
                // effective type of zero-arg function is its return type
                m_value_type = new_signature->return_type;
                v.partial_args = 0;
            } else {
                if (v.partial_args != 0) {
                    // partial function call
                    if (v.definition != nullptr) {
                        v.partial_index = v.definition->symbol()->index();
                    } else {
                        SymbolTable& fn_symtab = m_function.symtab().add_child("?/partial");
                        auto fn = make_unique<Function>(module(), fn_symtab);
                        v.partial_index = module().add_function(move(fn));
                    }
                    auto& fn = module().get_function(v.partial_index);
                    fn.signature() = *new_signature;
                    fn.signature().nonlocals.clear();
                    fn.signature().partial.clear();
                    for (const auto& arg : m_call_args) {
                        fn.add_partial(TypeInfo{arg.type_info});
                    }
                    assert(!fn.detect_generic());
                    fn.set_compiled();
                }
                m_value_type = TypeInfo{new_signature};
            }
        }
        m_call_args.clear();
    }

    void visit(ast::OpCall& v) override {
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        v.cond->apply(*this);
        if (m_value_type != ti_bool())
            throw ConditionNotBool();
        v.then_expr->apply(*this);
        auto then_type = m_value_type;
        v.else_expr->apply(*this);
        auto else_type = m_value_type;
        if (then_type != else_type) {
            throw BranchTypeMismatch(then_type, else_type);
        }
    }

    void visit(ast::WithContext& v) override {
        // resolve type of context (StructInit leads to incomplete struct type)
        v.context->apply(*this);
        // lookup the enter function with the resolved context type
        m_call_args.push_back({m_value_type, v.context->source_loc});
        m_call_ret = ti_unknown();
        v.enter_function.apply(*this);
        m_call_args.clear();
        assert(m_value_type.is_callable());
        auto enter_sig = m_value_type.signature();
        // re-resolve type of context (match actual struct type as found by resolving `with` function)
        m_type_info = enter_sig.params[0];
        m_cast_type = {};
        v.context->apply(*this);
        assert(m_value_type == m_type_info);
        // lookup the leave function, it's arg type is same as enter functions return type
        v.leave_type = enter_sig.return_type.effective_type();
        m_call_args.push_back({v.leave_type, v.context->source_loc});
        m_call_ret = ti_void();
        v.leave_function.apply(*this);
        m_call_args.clear();
        // resolve type of expression - it's also the type of the whole "with" expression
        v.expression->apply(*this);
        v.expression_type = m_value_type.effective_type();
    }

    void visit(ast::Function& v) override {
        // specified type (left hand side of '=')
        TypeInfo specified_type;
        if (v.definition) {
            specified_type = move(m_type_info);
        }
        // lambda type (right hand side of '=')
        v.type.apply(*this);
        assert(m_type_info);
        // fill in / check type from specified type
        if (specified_type.is_callable()) {
            if (!m_type_info.signature().return_type && specified_type.signature().return_type)
                m_type_info.signature().set_return_type(specified_type.signature().return_type);
            size_t idx = 0;
            auto& params = m_type_info.signature().params;
            for (const auto& sp : specified_type.signature().params) {
                if (idx >= params.size())
                    params.emplace_back(sp);
                else if (!params[idx])
                    params[idx] = sp;
                // specified param must match now
                if (params[idx] != sp)
                    throw DefinitionParamTypeMismatch(idx, sp, params[idx]);
                ++idx;
            }
        }
        m_value_type = move(m_type_info);
        v.call_args = m_call_args.size();

        Function& fn = module().get_function(v.index);
        fn.set_signature(m_value_type.signature_ptr());
        if (fn.detect_generic()) {
            // try to instantiate the specialization
            if (m_call_args.size() == fn.signature().params.size()) {
                // immediately called generic function -> specialize to normal function
                fn.set_compiled();
                specialize_to_call_args(fn, v.body, v.source_loc);
                m_value_type = TypeInfo{fn.signature_ptr()};
            } else if (v.definition) {
                // mark as generic, uncompiled
                fn.set_ast(v.body);
            } else
                throw UnexpectedGenericFunction(v.source_loc);
        } else {
            fn.set_compiled();
            // compile body and resolve return type
            if (v.definition) {
                // in case the function is recursive, propagate the type upwards
                auto symptr = v.definition->variable.identifier.symbol;
                auto& fn_dfn = module().get_function(symptr->index());
                fn_dfn.set_signature(m_value_type.signature_ptr());
            }
            resolve_types(fn, v.body);
            // if the return type is still Unknown, change it to Void (the body is empty)
            if (fn.signature().return_type.is_unknown())
                fn.signature().set_return_type(ti_void());
            m_value_type = TypeInfo{fn.signature_ptr()};
        }

        // parameterless function is equivalent to its return type (eager evaluation)
       /* while (m_value_type.is_callable() && m_value_type.signature().params.empty()) {
            m_value_type = m_value_type.signature().return_type;
        }*/
        // check specified type again - in case it wasn't Function
        if (!m_value_type.is_callable() && specified_type) {
            if (m_value_type != specified_type)
                throw DefinitionTypeMismatch(specified_type, m_value_type, v.source_loc);
        }
    }

    // The cast expression is translated to a call to `cast` method from the Cast class.
    // The inner expression type and the cast type are used to lookup the instance of Cast.
    void visit(ast::Cast& v) override {
        // resolve the target type -> m_type_info
        v.type->apply(*this);
        v.to_type = std::move(m_type_info);  // save for fold_const_expr
        // resolve the inner expression -> m_value_type
        // (the Expression might use the specified type from `m_cast_type`)
        m_cast_type = v.to_type;
        v.expression->apply(*this);
        m_cast_type = {};
        v.from_type = move(m_value_type);
        // cast to Void -> don't call the cast function, just drop the expression result from stack
        // cast to the same type -> noop
        if (v.to_type.is_void() || v.from_type == v.to_type) {
            v.cast_function.reset();
            m_value_type = v.to_type;
            return;
        }
        // lookup the cast function with the resolved arg/return types
        m_call_args.push_back({v.from_type, v.expression->source_loc});
        m_call_ret = v.to_type;
        v.cast_function->apply(*this);
        // set the effective type of the Cast expression and clean the call types
        m_value_type = std::move(m_call_ret);
        m_call_args.clear();
    }

    void visit(ast::TypeName& t) final {
        switch (t.symbol->type()) {
            case Symbol::TypeName:
                m_type_info = t.symbol.symtab()->module()->get_type(t.symbol->index());
                break;
            case Symbol::TypeVar:
                if (t.symbol.symtab() == &m_function.symtab()
                && t.symbol->index() <= m_type_args.size())
                    m_type_info = m_type_args[t.symbol->index() - 1];
                else
                    m_type_info = TypeInfo{ TypeInfo::Var(t.symbol->index()) };
                break;
            default:
                break;
        }
    }

    void visit(ast::FunctionType& t) final {
        auto signature = std::make_shared<Signature>();
        for (const auto& p : t.params) {
            if (p.type)
                p.type->apply(*this);
            else
                m_type_info = ti_unknown();
            signature->add_parameter(move(m_type_info));
        }
        if (t.result_type)
            t.result_type->apply(*this);
        else
            m_type_info = ti_unknown();
        signature->set_return_type(m_type_info);
        m_type_info = TypeInfo{std::move(signature)};
    }

    void visit(ast::ListType& t) final {
        t.elem_type->apply(*this);
        m_type_info = ti_list(move(m_type_info));
    }

    void visit(ast::TupleType& t) final {
        std::vector<TypeInfo> subtypes;
        for (auto& st : t.subtypes) {
            st->apply(*this);
            subtypes.push_back(move(m_type_info));
        }
        m_type_info = TypeInfo{move(subtypes)};
    }

private:
    Module& module() { return m_function.module(); }

    Index get_type_id(TypeInfo&& type_info) {
        // is the type builtin?
        Index type_id = BuiltinModule::static_instance().find_type(type_info);
        if (type_id >= 32) {
            // add to current module
            type_id = 32 + module().add_type(move(type_info));
        }
        return type_id;
    }

    Index get_type_id(const TypeInfo& type_info) {
        // is the type builtin?
        Index type_id = BuiltinModule::static_instance().find_type(type_info);
        if (type_id >= 32) {
            // add to current module
            type_id = 32 + module().add_type(type_info);
        }
        return type_id;
    }

    void specialize_arg(const TypeInfo& sig, const TypeInfo& arg,
                        std::vector<TypeInfo>& resolved,
                        const std::function<void(const TypeInfo& exp, const TypeInfo& got)>& exc_cb) const
    {
        switch (sig.type()) {
            case Type::Unknown: {
                auto var = sig.generic_var();
                assert(var > 0);
                if (resolved.size() < var)
                    resolved.resize(var);
                if (!resolved[var-1])
                    resolved[var-1] = arg;
                else if (resolved[var-1] != arg)
                    exc_cb(resolved[var-1], arg);
                break;
            }
            case Type::List:
                if (arg.type() != Type::List)
                    exc_cb(sig, arg);
                return specialize_arg(sig.elem_type(), arg.elem_type(), resolved, exc_cb);
            case Type::Function:
            case Type::Tuple:
                assert(!"not implemented");
                break;
            default:
                // Int32 etc. (never generic)
                break;
        }
    }

    void resolve_generic_type(const std::vector<TypeInfo>& resolved, TypeInfo& sig) const
    {
        switch (sig.type()) {
            case Type::Unknown: {
                auto var = sig.generic_var();
                assert(var > 0);
                if (var <= resolved.size())
                    sig = resolved[var - 1];
                break;
            }
            case Type::List: {
                TypeInfo elem_type = sig.elem_type();
                resolve_generic_type(resolved, elem_type);
                sig = ti_list(move(elem_type));
                break;
            }
            case Type::Function:
            case Type::Tuple:
                assert(!"not implemented");
                break;
            default:
                // Int32 etc. (never generic)
                break;
        }
    }

    // Specialize a generic function:
    // * use m_call_args to resolve actual types of type variable
    // * resolve function body (deduce actual return type)
    // * use the deduced return type to resolve type variables in generic return type
    // Modifies `fn` in place - it should be already copied.
    // Throw when the signature doesn't match the call args or deduced return type.
    void specialize_to_call_args(Function& fn, const ast::Block& body, const SourceLocation& loc) const
    {
        std::vector<TypeInfo> resolved_types;
        for (size_t i = 0; i < fn.signature().params.size(); i++) {
            const auto& arg = m_call_args[i];
            auto& out_type = fn.signature().params[i];
            if (arg.type_info.is_unknown())
                continue;
            specialize_arg(out_type, arg.type_info, resolved_types,
                    [i, &loc](const TypeInfo& exp, const TypeInfo& got) {
                        throw UnexpectedArgumentType(i, exp, got, loc);
                    });
        }
        // resolve generic vars to received types
        for (auto & out_type : fn.signature().params) {
            resolve_generic_type(resolved_types, out_type);
        }
        resolve_generic_type(resolved_types, fn.signature().return_type);
        // resolve function body to get actual return type
        auto sig_ret = fn.signature().return_type;
        resolve_types(fn, body, resolved_types);
        auto deduced_ret = fn.signature().return_type;
        // resolve generic return type
        if (!deduced_ret.is_unknown())
            specialize_arg(sig_ret, deduced_ret, resolved_types,
                           [](const TypeInfo& exp, const TypeInfo& got) {
                               throw UnexpectedReturnType(exp, got);
                           });
        resolve_generic_type(resolved_types, sig_ret);
        fn.signature().return_type = sig_ret;
    }

    struct Specialized {
        TypeInfo type_info;
        Index index;
    };

    /// Given a generic function, create a copy and specialize it to call args.
    /// * create a copy of original generic function in this module
    /// * copy function's AST
    /// * keep original symbol table (with relative references, like parameter #1 at depth -2)
    /// Symbols in copied AST still point to original generic function.
    /// \param symptr   Pointer to symbol pointing to original function
    std::optional<Specialized> specialize_function(SymbolPointer symptr, const SourceLocation& loc)
    {
        auto& fn = symptr.get_function();
        if (!fn.detect_generic())
            return {};  // not generic, nothing to specialize
        auto fspec = make_unique<Function>(module(), fn.symtab());
        fspec->set_signature(std::make_shared<Signature>(fn.signature()));  // copy, not ref
        fspec->set_ast(fn.ast());
        fspec->ensure_ast_copy();
        specialize_to_call_args(*fspec, fspec->ast(), loc);
        auto fspec_sig = fspec->signature_ptr();
        // Copy original symbol and set it to the specialized function
        auto fspec_idx = module().add_function(move(fspec));
        auto res = std::make_optional<Specialized>({
                TypeInfo{fspec_sig},
                fspec_idx
        });
        assert(!symptr->ref());
        assert(symptr->depth() == 0);
        // add to specialized functions in this module
        module().add_spec_function(symptr, fspec_idx);
        return res;
    }

    // Consume params from `orig_signature` according to `m_call_args`, creating new signature
    std::shared_ptr<Signature> resolve_params(const Signature& orig_signature, ast::Call& v) const
    {
        auto res = std::make_shared<Signature>(orig_signature);
        int i = 0;
        for (const auto& arg : m_call_args) {
            i += 1;
            // check there are more params to consume
            while (res->params.empty()) {
                if (res->return_type.type() == Type::Function) {
                    // collapse returned function, start consuming its params
                    res = std::make_unique<Signature>(res->return_type.signature());
                    ++v.wrapped_execs;
                    v.partial_args = 0;
                } else {
                    throw UnexpectedArgument(i, arg.source_loc);
                }
            }
            // check type of next param
            if (res->params[0] != arg.type_info) {
                throw UnexpectedArgumentType(i, res->params[0], arg.type_info, arg.source_loc);
            }
            // consume next param
            ++ v.partial_args;
            if (v.wrapped_execs != 0 && !res->has_closure())
                v.wrapped_execs = 1;
            res->params.erase(res->params.begin());
        }
        return res;
    }

    static const char* match_to_cstr(Match m) {
        switch (m) {
            case Match::None: return "[ ]";
            case Match::Generic: return "[?]";
            case Match::Partial: return "[~]";
            case Match::Exact: return "[x]";
        }
        UNREACHABLE;
    }

    /// \returns Match: None/Generic/Partial/Exact
    /// Partial match means the signature has less parameters than call args.
    Match match_params(const Signature& signature) const
    {
        auto sig = std::make_unique<Signature>(signature);
        int i = 0;
        Match res = Match::Partial;
        for (const auto& arg : m_call_args) {
            i += 1;
            // check there are more params to consume
            while (sig->params.empty()) {
                if (sig->return_type.type() == Type::Function) {
                    // collapse returned function, start consuming its params
                    sig = std::make_unique<Signature>(sig->return_type.signature());
                } else {
                    // unexpected argument
                    return Match::None;
                }
            }
            // check type of next param
            auto m = match_type(arg.type_info, sig->params[0]);
            switch (m) {
                case Match::None: return m;
                case Match::Generic: res = m; break;
                default: break;
            }
            // consume next param
            sig->params.erase(sig->params.begin());
        }
        if (sig->params.empty() && res == Match::Partial)
            return Match::Exact;
        return res;
    }

    // Match call args with signature (which contains type vars T, U...)
    // Throw if unmatched, return resolved types for T, U... if matched
    // The result types are in the same order as the matched type vars in signature,
    // e.g. for `class MyClass T U V { my V U -> T }` it will return actual types [T, U, V].
    std::vector<TypeInfo> resolve_instance_types(const Signature& signature) const
    {
        const auto* sig = &signature;
        size_t i_arg = 0;
        size_t i_prm = 0;
        std::vector<TypeInfo> res;
        // optimization: Resize `res` to according to return type, which is usually the last type var
        if (signature.return_type.is_unknown()) {
            auto var = signature.return_type.generic_var();
            res.resize(var);
        }
        // resolve args
        for (const auto& arg : m_call_args) {
            i_arg += 1;
            // check there are more params to consume
            while (i_prm >= sig->params.size()) {
                if (sig->return_type.type() == Type::Function) {
                    // collapse returned function, start consuming its params
                    sig = &sig->return_type.signature();
                    i_prm = 0;
                } else {
                    // unexpected argument
                    throw UnexpectedArgument(i_arg, arg.source_loc);
                }
            }
            // resolve T (only from original signature)
            const auto& prm = sig->params[i_prm];
            if (prm.is_unknown() && sig == &signature) {
                auto var = prm.generic_var();
                assert(var != 0);
                // make space for additional type var in the result
                if (var > res.size())
                    res.resize(var);
                auto arg_type = arg.type_info.effective_type();
                if (res[var-1].is_unknown())
                    res[var-1] = arg_type;
                else if (res[var-1] != arg_type)
                    throw UnexpectedArgumentType(i_arg, res[var-1], arg_type,
                            arg.source_loc);
            }
            // check type of next param
            if (prm != arg.type_info) {
                throw UnexpectedArgumentType(i_arg, prm,
                        arg.type_info, arg.source_loc);
            }
            // consume next param
            ++i_prm;
        }
        // use m_call_ret only as a hint - if return type var is still unknown
        if (signature.return_type.is_unknown()) {
            auto var = signature.return_type.generic_var();
            assert(var != 0);
            if (res[var - 1].is_unknown())
                res[var - 1] = m_call_ret;
        }
        return res;
    }

private:
    Function& m_function;
    const std::vector<TypeInfo>& m_type_args;  // resolved type parameters

    TypeInfo m_type_info;   // resolved ast::Type
    TypeInfo m_value_type;  // inferred type of the value
    TypeInfo m_cast_type;   // target type of a Cast

    // signature for resolving overloaded functions and templates
    CallArgs m_call_args;  // actual argument types
    TypeInfo m_call_ret;   // expected return type

    Class* m_class = nullptr;
    Instance* m_instance = nullptr;
    bool m_intrinsic = false;
};


void resolve_types(Function& func, const ast::Block& block,
                   const std::vector<TypeInfo>& type_args)
{
    TypeCheckerVisitor visitor {func, type_args};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
    if (func.is_compiled() && func.signature().return_type.is_unknown())
        func.signature().return_type = ti_void();
}


} // namespace xci::script
