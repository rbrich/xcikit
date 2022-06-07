// resolve_types.cpp created on 2019-06-13 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "resolve_types.h"
#include <xci/script/Value.h>
#include <xci/script/Builtin.h>
#include <xci/script/Function.h>
#include <xci/script/Error.h>
#include <xci/compat/macros.h>

#include <range/v3/view/enumerate.hpp>

#include <sstream>
#include <optional>
#include <unordered_set>

namespace xci::script {

using std::stringstream;
using std::endl;

using ranges::views::enumerate;


class MatchScore {
public:
    MatchScore() = default;
    MatchScore(int8_t exact, int8_t coerce, int8_t generic)
        : m_exact(exact), m_coerce(coerce), m_generic(generic) {}
    explicit MatchScore(int8_t exact) : m_exact(exact) {}  // -1 => mismatch

    static MatchScore exact() { return MatchScore(1); }
    static MatchScore coerce() { return MatchScore(0, 1, 0); }
    static MatchScore generic() { return MatchScore(0, 0, 1); }
    static MatchScore mismatch() { return MatchScore(-1); }

    void add_exact() { ++m_exact; }
    void add_coerce() { ++m_coerce; }
    void add_generic() { ++m_generic; }

    bool is_exact() const { return m_exact >= 0 && (m_coerce + m_generic) == 0; }
    bool is_coerce() const { return m_coerce > 0; }

    explicit operator bool() const { return m_exact != -1; }
    auto operator<=>(const MatchScore&) const = default;

    MatchScore operator+(MatchScore rhs) {
        return {int8_t(m_exact + rhs.m_exact),
                int8_t(m_coerce + rhs.m_coerce),
                int8_t(m_generic + rhs.m_generic)};
    }

    void operator+=(MatchScore rhs) {
        m_exact += rhs.m_exact;  // NOLINT
        m_coerce += rhs.m_coerce;  // NOLINT
        m_generic += rhs.m_generic;  // NOLINT
    }

    friend std::ostream& operator<<(std::ostream& os, MatchScore v) {
        if (!v)
            return os << "[ ]";
        os << '[' << int(v.m_exact);
        if (v.m_coerce != 0) os << '~' << int(v.m_coerce);
        if (v.m_generic != 0) os << '?' << int(v.m_generic);
        return os << ']';
    }

private:
    int8_t m_exact = 0;  // num parameters matched exactly (Int == Int)
    int8_t m_coerce = 0;  // num parameters that can coerce (Int32 => Int64)
    int8_t m_generic = 0;  // num parameters matched generically (T == T or T == Int or Num T == Int)
};

static MatchScore match_params(const std::vector<TypeInfo>& candidate, const std::vector<TypeInfo>& actual);
static MatchScore match_type(const TypeInfo& candidate, const TypeInfo& actual);
static MatchScore match_tuple(const TypeInfo& candidate, const TypeInfo& actual);
static MatchScore match_struct(const TypeInfo& candidate, const TypeInfo& actual);
static MatchScore match_tuple_to_struct(const TypeInfo& candidate, const TypeInfo& actual);


class TypeCheckHelper {
public:
    explicit TypeCheckHelper(TypeInfo&& spec) : m_spec(std::move(spec)) {}
    TypeCheckHelper(TypeInfo&& spec, TypeInfo&& cast) : m_spec(std::move(spec)), m_cast(std::move(cast)) {}

    TypeInfo resolve(const TypeInfo& inferred, const SourceLocation& loc) const {
        // struct - resolve to either specified or cast type
        const TypeInfo& ti = eval_type();
        if (ti.is_struct()) {
            if (inferred.is_struct()) {
                if (!match_struct(inferred, ti))
                    throw DefinitionTypeMismatch(ti, inferred, loc);
                return ti;
            }
            if (inferred.is_tuple()) {
                if (!match_tuple_to_struct(inferred, ti))
                    throw DefinitionTypeMismatch(ti, inferred, loc);
                return ti;
            }
        }
        // otherwise, resolve to specified type, ignore cast type (a cast function will be called)
        if (!m_spec)
            return inferred;
        if (!match_type(inferred, m_spec))
            throw DefinitionTypeMismatch(m_spec, inferred, loc);
        return m_spec;
    }

    void check_struct_item(const std::string& key, const TypeInfo& inferred, const SourceLocation& loc) const {
        assert(eval_type().is_struct());
        const auto& spec_items = eval_type().struct_items();
        auto spec_it = std::find_if(spec_items.begin(), spec_items.end(),
            [&key](const TypeInfo::StructItem& spec) {
              return spec.first == key;
            });
        if (spec_it == spec_items.end())
            throw StructUnknownKey(eval_type(), key, loc);
        if (!match_type(inferred, spec_it->second))
            throw StructKeyTypeMismatch(eval_type(), spec_it->second, inferred, loc);
    }

    const TypeInfo& spec() const { return m_spec; }
    TypeInfo&& spec() { return std::move(m_spec); }

    const TypeInfo& cast() const { return m_cast; }
    TypeInfo&& cast() { return std::move(m_cast); }

    const TypeInfo& eval_type() const { return m_cast ? m_cast : m_spec; }
    TypeInfo&& eval_type() { return m_cast ? std::move(m_cast) : std::move(m_spec); }

private:
    TypeInfo m_spec;  // specified type
    TypeInfo m_cast;  // casted-to type
};


static MatchScore match_params(const std::vector<TypeInfo>& candidate, const std::vector<TypeInfo>& actual)
{
    if (candidate.size() != actual.size())
        return MatchScore(-1);
    MatchScore score;
    for (size_t i = 0; i != actual.size(); ++i) {
        auto m = match_type(candidate[i], actual[i]);
        if (!m || m.is_coerce())
            return MatchScore(-1);
        score += m;
    }
    return score;
}


/// \returns MatchScore: mismatch/generic/exact or combination in case of complex types
static MatchScore match_type(const TypeInfo& candidate, const TypeInfo& actual)
{
    if (candidate.is_struct() && actual.is_struct())
        return match_struct(candidate, actual);
    if (candidate.is_tuple() && actual.is_tuple())
        return match_tuple(candidate, actual);
    if (candidate.is_named() || actual.is_named())
        return MatchScore::coerce() + match_type(candidate.underlying(), actual.underlying());
    if (candidate == actual) {
        if (actual.is_generic() || candidate.is_generic())
            return MatchScore::generic();
        else
            return MatchScore::exact();
    }
    return MatchScore::mismatch();
}


/// Match tuple to tuple
/// \param candidate    Candidate tuple type
/// \param actual       Actual resolved tuple type for the value
/// \returns Total match score of all fields, or mismatch
static MatchScore match_tuple(const TypeInfo& candidate, const TypeInfo& actual)
{
    assert(candidate.is_tuple());
    assert(actual.is_tuple());
    const auto& actual_types = actual.subtypes();
    const auto& candidate_types = candidate.subtypes();
    if (candidate_types.size() != actual_types.size())
        return MatchScore::mismatch();  // number of fields doesn't match
    if (candidate == actual)
        return MatchScore::exact();
    MatchScore res;
    if (candidate.is_named() || actual.is_named())
        res.add_coerce();
    auto actual_iter = actual_types.begin();
    for (const auto& inf_type : candidate_types) {
        auto m = match_type(inf_type, *actual_iter);
        if (!m)
            return MatchScore::mismatch();  // item type doesn't match
        res += m;
        ++actual_iter;
    }
    return res;
}


/// Match incomplete Struct type from ast::StructInit to resolved Struct type.
/// All keys and types from inferred are checked against resolved.
/// Partial match is possible when inferred has less keys than resolved.
/// \param candidate    Possibly incomplete Struct type as constructed from AST
/// \param actual       Actual resolved type for the value
/// \returns Total match score of all fields, or mismatch
static MatchScore match_struct(const TypeInfo& candidate, const TypeInfo& actual)
{
    assert(candidate.is_struct());
    assert(actual.is_struct());
    const auto& actual_items = actual.struct_items();
    if (candidate == actual)
        return MatchScore::exact();
    MatchScore res;
    if (candidate.is_named() || actual.is_named()) {
        // The named type doesn't match.
        // The underlying type may match - each field adds another match to total score.
        res.add_coerce();
    }
    for (const auto& inf : candidate.struct_items()) {
        auto act_it = std::find_if(actual_items.begin(), actual_items.end(),
                [&inf](const TypeInfo::StructItem& act) {
                  return act.first == inf.first;
                });
        if (act_it == actual_items.end())
            return MatchScore::mismatch();  // not found
        // check item type
        auto m = match_type(inf.second, act_it->second);
        if (!m)
            return MatchScore::mismatch();  // item type doesn't match
        res += m;
    }
    return res;
}


/// Match tuple to resolved Struct type, i.e. initialize struct with tuple literal
/// \param candidate    Tuple with same or lesser number of fields
/// \param actual       Actual resolved struct type for the value
/// \returns Total match score of all fields, or mismatch
static MatchScore match_tuple_to_struct(const TypeInfo& candidate, const TypeInfo& actual)
{
    assert(candidate.is_tuple());
    assert(actual.is_struct());
    const auto& actual_items = actual.struct_items();
    const auto& candidate_types = candidate.subtypes();
    if (candidate_types.size() > actual_items.size())
        return MatchScore::mismatch();  // number of fields doesn't match
    if (candidate == actual)
        return MatchScore::exact();
    MatchScore res;
    if (candidate.is_named() || actual.is_named())
        res.add_coerce();
    auto actual_iter = actual_items.begin();
    for (const auto& inf_type : candidate_types) {
        auto m = match_type(inf_type, actual_iter->second);
        if (!m)
            return MatchScore::mismatch();  // item type doesn't match
        res += m;
        ++actual_iter;
    }
    return res;
}


struct Candidate {
    Module* module;
    Index index;
    SymbolPointer symptr;
    TypeInfo type;
    MatchScore match;
};


/// Find best match from candidates
static std::pair<const Candidate*, bool> find_best_candidate(const std::vector<Candidate>& candidates)
{
    bool conflict = false;
    MatchScore score(-1);
    const Candidate* found = nullptr;
    for (const auto& item : candidates) {
        if (!item.match)
            continue;
        if (item.match > score) {
            // found better match
            score = item.match;
            found = &item;
            conflict = false;
            continue;
        }
        if (item.match == score) {
            // found same match -> conflict
            conflict = true;
        }
    }
    return {found, conflict};
}


class TypeCheckerVisitor final: public ast::Visitor {
    struct CallArg {
        TypeInfo type_info;
        SourceLocation source_loc;
    };
    using CallArgs = std::vector<CallArg>;

public:
    explicit TypeCheckerVisitor(Function& func) : m_function(func) {}

    void visit(ast::Definition& dfn) override {
        // Evaluate specified type
        if (dfn.variable.type) {
            dfn.variable.type->apply(*this);
        } else {
            m_type_info = {};
        }

        if (m_class != nullptr) {
            const auto& psym = dfn.symbol();
            m_class->add_function(psym->index());
        }

        if (m_instance != nullptr) {
            // evaluate type according to class and type vars
            const auto& psym = dfn.symbol();
            const auto& cls_fn = psym->ref().get_function();
            TypeInfo eval_type {cls_fn.signature_ptr()};
            for (const auto&& [i, t] : m_instance->types() | enumerate)
                eval_type.replace_var(i + 1, t);

            // specified type is basically useless here, let's just check
            // it matches evaluated type from class instance
            if (m_type_info && m_type_info != eval_type)
                throw DefinitionTypeMismatch(m_type_info, eval_type, dfn.expression->source_loc);

            m_type_info = std::move(eval_type);

            auto idx_in_cls = m_instance->class_().get_function_index(psym->ref()->index());
            m_instance->set_function(idx_in_cls, psym->index(), psym);
        }

        // Expression might use the specified type from `m_type_info`
        if (dfn.expression) {
            dfn.expression->definition = &dfn;
            dfn.expression->apply(*this);
        } else {
            // declaration: use specified type directly
            m_value_type = std::move(m_type_info);
        }

        Function& func = module().get_function(dfn.symbol()->index());
        if (m_value_type.is_callable())
            func.signature() = m_value_type.signature();
        else {
            const auto& source_loc = dfn.expression ?
                    dfn.expression->source_loc : dfn.variable.identifier.source_loc;
            resolve_return_type(func.signature(), m_value_type, source_loc);
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
            m_instance->add_type(std::move(m_type_info));
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
        Index index = module().add_type(TypeInfo{v.type_name.name, std::move(m_type_info)});
        v.type_name.symbol->set_index(index);
    }

    void visit(ast::TypeAlias& v) override {
        v.type->apply(*this);

        // add the actual type to Module, referenced by symbol
        Index index = module().add_type(std::move(m_type_info));
        v.type_name.symbol->set_index(index);
    }

    void visit(ast::Literal& v) override {
        TypeCheckHelper type_check(std::move(m_type_info));
        m_value_type = type_check.resolve(v.value.type_info(), v.source_loc);
    }

    void visit(ast::Tuple& v) override {
        TypeCheckHelper type_check(std::move(m_type_info), std::move(m_cast_type));
        // build TypeInfo from subtypes
        std::vector<TypeInfo> subtypes;
        subtypes.reserve(v.items.size());
        for (auto& item : v.items) {
            item->apply(*this);
            subtypes.push_back(m_value_type.effective_type());
        }
        m_value_type = type_check.resolve(TypeInfo(std::move(subtypes)), v.source_loc);
    }

    void visit(ast::List& v) override {
        TypeCheckHelper type_check(std::move(m_type_info), std::move(m_cast_type));
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
        if (m_value_type.is_generic() && type_check.eval_type())
            m_value_type = std::move(type_check.eval_type());
        // FIXME: allow generic type: fun <T> Void->[T] { []:[T] }
        if (m_value_type.elem_type().is_generic())
            throw MissingExplicitType(v.source_loc);
        v.elem_type_id = get_type_id(m_value_type.elem_type());
    }

    void visit(ast::StructInit& v) override {
        if (!v.struct_type.is_unknown()) {
            // second pass (from ast::WithContext):
            // * v.struct_type is the inferred type
            // * m_type_info is the final struct type
            if (m_type_info.is_unknown()) {
                m_value_type = v.struct_type;
                return;
            }
            if (!m_type_info.is_struct())
                throw StructTypeMismatch(m_type_info, v.source_loc);
            if (!match_struct(v.struct_type, m_type_info))
                throw StructTypeMismatch(m_type_info, v.source_loc);
            v.struct_type = std::move(m_type_info);
            m_value_type = v.struct_type;
            return;
        }
        // first pass - resolve incomplete struct type
        //              and check it matches specified type (if any)
        TypeCheckHelper type_check(std::move(m_type_info), std::move(m_cast_type));
        const auto& specified = type_check.eval_type();
        if (!specified.is_unknown() && !specified.is_struct())
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
            if (!specified.is_unknown())
                type_check.check_struct_item(item.first.name, item_type, item.second->source_loc);
            ti_items.emplace_back(item.first.name, item_type);
        }
        v.struct_type = TypeInfo(std::move(ti_items));
        if (!specified.is_unknown()) {
            assert(match_struct(v.struct_type, specified));  // already checked above
            v.struct_type = std::move(type_check.eval_type());
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
                    const auto& type_args = m_function.signature().type_args;
                    if (var > 0 && var <= type_args.size())
                        m_type_info = type_args[var-1];
                    else {
                        // unresolved -> unknown type id
                        m_value_type = {};
                        break;
                    }
                }
                // Record the resolved Type ID for Compiler
                v.index = get_type_id(std::move(m_type_info));
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
                Index cls_fn_idx = cls.get_function_index(sym.ref()->index());
                const auto& cls_fn = symmod.get_function(sym.ref()->index());
                auto inst_types = resolve_instance_types(cls_fn.signature());
                // find instance using resolved T
                std::vector<Candidate> candidates;
                auto inst_psym = v.chain;
                while (inst_psym) {
                    assert(inst_psym->type() == Symbol::Instance);
                    auto* inst_mod = inst_psym.symtab()->module();
                    if (inst_mod == nullptr)
                        inst_mod = &module();
                    auto& inst = inst_mod->get_instance(inst_psym->index());
                    auto inst_fn = inst.get_function(cls_fn_idx);
                    auto m = match_params(inst.types(), inst_types);
                    candidates.push_back({inst_mod, inst_fn.index, inst_psym, TypeInfo{}, m});
                    inst_psym = inst_psym->next();
                }

                auto [found, conflict] = find_best_candidate(candidates);

                if (found && !conflict) {
                    auto spec_idx = specialize_instance(found->symptr, cls_fn_idx, v.identifier.source_loc);
                    if (spec_idx != no_index) {
                        auto& inst = module().get_instance(spec_idx);
                        auto inst_fn_idx = inst.get_function(cls_fn_idx).index;
                        v.module = &module();
                        v.index = inst_fn_idx;
                    } else {
                        v.module = found->module;
                        v.index = found->index;
                    }
                    auto& fn = v.module->get_function(v.index);
                    m_value_type = TypeInfo{fn.signature_ptr()};
                    break;
                }

                // ERROR couldn't find single matching instance for `args`
                stringstream o_candidates;
                for (const auto& c : candidates) {
                    auto& fn = c.module->get_function(c.index);
                    o_candidates << "   " << c.match << "  "
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
                if (conflict)
                    throw FunctionConflict(v.identifier.name, o_ftype.str(), o_candidates.str(), v.identifier.source_loc);
                else
                    throw FunctionNotFound(v.identifier.name, o_ftype.str(), o_candidates.str(), v.identifier.source_loc);
            }
            case Symbol::Function: {
                // specified type in definition
                if (v.definition && m_type_info) {
                    assert(m_call_args.empty());
                    if (m_type_info.is_callable()) {
                        for (const auto& t : m_type_info.signature().params)
                            m_call_args.push_back({t, v.source_loc});
                        m_call_ret = m_type_info.signature().return_type;
                        m_type_info = {};
                    } else {
                        // A naked type, consider it a function return type
                        m_call_ret = std::move(m_type_info);
                    }
                }

                auto res = resolve_overload(v.identifier.symbol, v.identifier);
                // The referenced function must have been defined
                if (!res.type.effective_type())
                    throw MissingExplicitType(v.identifier.name, v.identifier.source_loc);
                v.module = res.module;
                v.index = res.index;
                m_value_type = res.type;

                if (v.definition) {
                    m_call_args.clear();
                    m_call_ret = {};
                }
                break;
            }
            case Symbol::Module:
                m_value_type = TypeInfo{Type::Module};
                break;
            case Symbol::Nonlocal: {
                assert(sym.ref());
                auto nl_sym = sym.ref();
                switch (nl_sym->type()) {
                    case Symbol::Parameter: {
                        // owning function of the nonlocal symbol
                        auto* nl_owner = nl_sym.symtab()->function();
                        assert(nl_owner != nullptr);
                        m_value_type = nl_owner->parameter(nl_sym->index());
                        break;
                    }
                    case Symbol::Function: {
                        auto res = resolve_overload(nl_sym, v.identifier);
                        v.module = res.module;
                        v.index = res.index;
                        m_value_type = res.type;
                        break;
                    }
                    default:
                        assert(!"non-local must reference a parameter or a function");
                        return;
                }
                m_function.set_nonlocal(sym.index(), TypeInfo{m_value_type});
                break;
            }
            case Symbol::Parameter:
                m_value_type = m_function.parameter(sym.index());
                break;
            case Symbol::Value:
                if (sym.index() == no_index) {
                    m_intrinsic = true;
                    // __value - expects a single parameter
                    if (m_call_args.size() != 1)
                        throw UnexpectedArgumentCount(1, m_call_args.size(), v.source_loc);
                    // cleanup - args are now fully processed
                    m_call_args.clear();
                    // __value returns index (Int32)
                    m_value_type = ti_int32();
                    break;
                }
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
        TypeCheckHelper type_check(std::move(m_type_info), std::move(m_cast_type));

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
        m_call_ret = std::move(type_check.eval_type());
        m_intrinsic = false;

        // using resolved args, resolve the callable itself
        // (it may use args types for overload resolution)
        v.callable->apply(*this);
        v.intrinsic = m_intrinsic;

        if (!m_value_type.is_unknown() && !m_value_type.is_callable() && !m_call_args.empty()) {
            throw UnexpectedArgument(1, m_call_args[0].source_loc);
        }

        if (m_value_type.is_callable()) {
            // result is new signature with args removed (applied)
            auto new_signature = consume_params_from_call_args(m_value_type.signature(), v);
            if (new_signature->params.empty()) {
                if (v.definition == nullptr) {
                    // effective type of zero-arg function is its return type
                    m_value_type = new_signature->return_type;
                } else {
                    // Not really calling, just defining, e.g. `f = compose u v`
                    // Keep the return type as is, making it `Void -> <lambda type>`
                    m_value_type = TypeInfo{new_signature};
                }
                v.partial_args = 0;
            } else {
                if (v.partial_args != 0) {
                    // partial function call
                    if (v.definition != nullptr) {
                        v.partial_index = v.definition->symbol()->index();
                    } else {
                        SymbolTable& fn_symtab = m_function.symtab().add_child("?/partial");
                        Function fn {module(), fn_symtab};
                        v.partial_index = module().add_function(std::move(fn)).index;
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
        m_call_ret = {};
    }

    void visit(ast::OpCall& v) override {
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        TypeInfo expr_type;
        for (auto& item : v.if_then_expr) {
            item.first->apply(*this);
            if (m_value_type != ti_bool())
                throw ConditionNotBool();
            item.second->apply(*this);
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
        m_call_ret = {};
        // resolve type of expression - it's also the type of the whole "with" expression
        v.expression->apply(*this);
        v.expression_type = m_value_type.effective_type();
    }

    void visit(ast::Function& v) override {
        Function& fn = module().get_function(v.index);
        // specified type (left-hand side of '=')
        TypeInfo specified_type;
        if (v.definition) {
            specified_type = std::move(m_type_info);
            // declared type (decl statement)
            if (fn.signature()) {
                TypeInfo declared_type(fn.signature_ptr());
                if (specified_type && declared_type != specified_type) {
                    throw DeclarationTypeMismatch(declared_type, specified_type, v.source_loc);
                }
                specified_type = std::move(declared_type);
            }
        }
        // lambda type (right hand side of '=')
        v.type.apply(*this);
        assert(m_type_info);
        if (!m_instance && specified_type && specified_type != m_type_info.effective_type())
            throw DeclarationTypeMismatch(specified_type, m_type_info, v.source_loc);
        // fill in types from specified function type
        if (specified_type.is_callable()) {
            if (m_type_info.signature().return_type.is_unknown() && specified_type.signature().return_type)
                m_type_info.signature().set_return_type(specified_type.signature().return_type);
            size_t idx = 0;
            auto& params = m_type_info.signature().params;
            for (const auto& sp : specified_type.signature().params) {
                if (idx >= params.size())
                    params.emplace_back(sp);
                else if (params[idx].is_unknown())
                    params[idx] = sp;
                // specified param must match now
                if (params[idx] != sp)
                    throw DefinitionParamTypeMismatch(idx, sp, params[idx]);
                ++idx;
            }
        }
        m_value_type = std::move(m_type_info);
        v.call_args = m_call_args.size();

        fn.set_signature(m_value_type.signature_ptr());
        if (fn.has_generic_params()) {
            // try to instantiate the specialization
            if (m_call_args.size() == fn.signature().params.size()) {
                // immediately called or returned generic function
                // -> try to specialize to normal function
                specialize_to_call_args(fn, v.body, v.source_loc);
                m_value_type = TypeInfo{fn.signature_ptr()};
            } else if (!v.definition) {
                resolve_types(fn, v.body);
                if (fn.detect_generic()) {
                    stringstream sig_str;
                    sig_str << fn.name() << ':' << fn.signature();
                    throw UnexpectedGenericFunction(sig_str.str(), v.source_loc);
                }
                m_value_type = TypeInfo{fn.signature_ptr()};
            }
        } else {
            // compile body and resolve return type
            if (v.definition) {
                // in case the function is recursive, propagate the type upwards
                auto symptr = v.definition->symbol();
                auto& fn_dfn = module().get_function(symptr->index());
                fn_dfn.set_signature(m_value_type.signature_ptr());
            }
            resolve_types(fn, v.body);
            // if the return type is still Unknown, change it to Void (the body is empty)
            if (fn.signature().return_type.is_unknown())
                fn.signature().set_return_type(ti_void());
            m_value_type = TypeInfo{fn.signature_ptr()};
        }

        if (fn.has_generic_params())
            fn.set_ast(v.body);
        else
            fn.set_compiled();

        // parameterless function is equivalent to its return type (eager evaluation)
        /* while (m_value_type.is_callable() && m_value_type.signature().params.empty()) {
            m_value_type = m_value_type.signature().return_type;
        }*/
        // check specified type again - in case it wasn't Function
        if (!m_value_type.is_callable() && !specified_type.is_unknown()) {
            if (m_value_type != specified_type)
                throw DefinitionTypeMismatch(specified_type, m_value_type, v.source_loc);
        }
    }

    // The cast expression is translated to a call to `cast` method from the Cast class.
    // The inner expression type and the cast type are used to look up the instance of Cast.
    void visit(ast::Cast& v) override {
        // resolve the target type -> m_type_info
        v.type->apply(*this);
        v.to_type = std::move(m_type_info);  // save for fold_const_expr
        // resolve the inner expression -> m_value_type
        // (the Expression might use the specified type from `m_cast_type`)
        m_cast_type = v.to_type;
        v.expression->apply(*this);
        m_cast_type = {};
        v.from_type = std::move(m_value_type);
        // Cast to Void -> don't call the cast function, just drop the expression result from stack
        // Cast to the same type or same underlying type (from/to a named type) -> noop
        if (v.to_type.is_void() || is_same_underlying(v.from_type.effective_type(), v.to_type)) {
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
        m_type_info = resolve_type_name(t.symbol);
    }

    void visit(ast::FunctionType& t) final {
        auto signature = std::make_shared<Signature>();
        for (const auto& p : t.params) {
            if (p.type)
                p.type->apply(*this);
            else
                m_type_info = ti_unknown();
            signature->add_parameter(std::move(m_type_info));
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
        m_type_info = ti_list(std::move(m_type_info));
    }

    void visit(ast::TupleType& t) final {
        std::vector<TypeInfo> subtypes;
        for (auto& st : t.subtypes) {
            st->apply(*this);
            subtypes.push_back(std::move(m_type_info));
        }
        m_type_info = TypeInfo{std::move(subtypes)};
    }

    void visit(ast::StructType& t) final {
        TypeInfo::StructItems items;
        for (auto& st : t.subtypes) {
            st.type->apply(*this);
            items.emplace_back(st.identifier.name, std::move(m_type_info));
        }
        m_type_info = TypeInfo{std::move(items)};
    }

private:
    Module& module() { return m_function.module(); }

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

    TypeInfo resolve_type_name(SymbolPointer symptr) {
        switch (symptr->type()) {
            case Symbol::TypeName:
                return symptr.symtab()->module()->get_type(symptr->index());
            case Symbol::TypeVar: {
                const auto& type_args = m_function.signature().type_args;
                if (symptr.symtab() == &m_function.symtab()
                    && symptr->index() <= type_args.size())
                    return type_args[symptr->index() - 1];
                return TypeInfo{ TypeInfo::Var(symptr->index()) };
            }
            case Symbol::Nonlocal:
                return resolve_type_name(symptr->ref());
            default:
                return {};
        }
    }

    void specialize_arg(const TypeInfo& sig, const TypeInfo& deduced,
                        std::vector<TypeInfo>& resolved,
                        const std::function<void(const TypeInfo& exp, const TypeInfo& got)>& exc_cb) const
    {
        switch (sig.type()) {
            case Type::Unknown: {
                auto var = sig.generic_var();
                if (var > 0) {
                    // make space for additional type var
                    if (resolved.size() < var)
                        resolved.resize(var);
                    if (resolved[var-1].is_unknown())
                        resolved[var-1] = deduced;
                    else if (resolved[var-1] != deduced)
                        exc_cb(resolved[var-1], deduced);
                }
                break;
            }
            case Type::List:
                if (deduced.type() != Type::List) {
                    exc_cb(sig, deduced);
                    break;
                }
                return specialize_arg(sig.elem_type(), deduced.elem_type(), resolved, exc_cb);
            case Type::Function:
                break;
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
                if (var > 0 && var <= resolved.size())
                    sig = resolved[var - 1];
                break;
            }
            case Type::List: {
                TypeInfo elem_type = sig.elem_type();
                resolve_generic_type(resolved, elem_type);
                sig = ti_list(std::move(elem_type));
                break;
            }
            case Type::Function:
                break;
            case Type::Tuple:
                assert(!"not implemented");
                break;
            default:
                // Int32 etc. (never generic)
                break;
        }
    }

    void resolve_type_vars(Signature& signature) const
    {
        for (auto& arg_type : signature.params) {
            resolve_generic_type(signature.type_args, arg_type);
        }
        resolve_generic_type(signature.type_args, signature.return_type);
    }

    // Check return type matches and set it to concrete type if it's generic.
    void resolve_return_type(Signature& sig, const TypeInfo& deduced, const SourceLocation& loc) const
    {
        if (sig.return_type.is_unknown()) {
            if (deduced.is_unknown() && !sig.is_generic())
                throw MissingExplicitType(loc);
            if (deduced.is_callable() && &sig == &deduced.signature())
                throw MissingExplicitType(loc);  // the return type is recursive!
            specialize_arg(sig.return_type, deduced, sig.type_args,
                    [](const TypeInfo& exp, const TypeInfo& got) {
                        throw UnexpectedReturnType(exp, got);
                    });
            resolve_type_vars(sig);  // fill in concrete types using new type var info
            sig.return_type = deduced;  // Unknown/var=0 not handled by resolve_type_vars
            return;
        }
        if (sig.return_type != deduced)
            throw UnexpectedReturnType(sig.return_type, deduced);
    }

    // Specialize a generic function:
    // * use m_call_args to resolve actual types of type variable
    // * resolve function body (deduce actual return type)
    // * use the deduced return type to resolve type variables in generic return type
    // Modifies `fn` in place - it should be already copied.
    // Throw when the signature doesn't match the call args or deduced return type.
    void specialize_to_call_args(Function& fn, const ast::Block& body, const SourceLocation& loc) const
    {
        auto& signature = fn.signature();
        for (size_t i = 0; i < std::min(signature.params.size(), m_call_args.size()); i++) {
            const auto& arg = m_call_args[i];
            const auto& out_type = signature.params[i];
            if (arg.type_info.is_unknown())
                continue;
            specialize_arg(out_type, arg.type_info, signature.type_args,
                    [i, &loc](const TypeInfo& exp, const TypeInfo& got) {
                        throw UnexpectedArgumentType(i+1, exp, got, loc);
                    });
        }
        // resolve generic vars to received types
        resolve_type_vars(signature);
        // resolve function body to get actual return type
        auto sig_ret = signature.return_type;
        resolve_types(fn, body);
        auto deduced_ret = signature.return_type;
        // resolve generic return type
        if (!deduced_ret.is_unknown())
            specialize_arg(sig_ret, deduced_ret, signature.type_args,
                           [](const TypeInfo& exp, const TypeInfo& got) {
                               throw UnexpectedReturnType(exp, got);
                           });
        resolve_generic_type(signature.type_args, sig_ret);
        signature.return_type = sig_ret;
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
        if (!fn.has_generic_params())
            return {};  // not generic, nothing to specialize
        if (fn.signature().params.size() > m_call_args.size())
            return {};  // not enough call args

        // Check already created specializations if one of them matches
        for (auto spec_idx : module().get_spec_functions(symptr)) {
            auto& spec_fn = module().get_function(spec_idx);
            const auto& spec_sig = spec_fn.signature_ptr();
            if (match_signature(*spec_sig).is_exact())
                return std::make_optional<Specialized>({
                        TypeInfo{spec_sig},
                        spec_idx
                });
        }

        Function fspec(module(), fn.symtab());
        fspec.set_signature(std::make_shared<Signature>(fn.signature()));  // copy, not ref
        fspec.set_ast(fn.ast());
        fspec.ensure_ast_copy();
        specialize_to_call_args(fspec, fspec.ast(), loc);
        auto fspec_sig = fspec.signature_ptr();
        // Copy original symbol and set it to the specialized function
        auto fspec_idx = module().add_function(std::move(fspec)).index;
        auto res = std::make_optional<Specialized>({
                TypeInfo{fspec_sig},
                fspec_idx
        });
        assert(symptr->depth() == 0);
        // add to specialized functions in this module
        module().add_spec_function(symptr, fspec_idx);
        return res;
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
        const auto& called_inst_fn = inst.get_function(cls_fn_idx).symptr.get_function();
        auto resolved_types = resolve_instance_types(called_inst_fn.signature());
        auto inst_types = inst.types();
        for (auto& it : inst_types)
            resolve_generic_type(resolved_types, it);

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
            auto specialized = specialize_function(fn_info.symptr, loc);
            if (specialized) {
                spec.set_function(i, specialized->index, fn_info.symptr);
            } else {
                spec.set_function(i, fn_info.index, fn_info.symptr);
            }
        }

        // add to specialized instance in this module
        auto spec_idx = module().add_instance(std::move(spec)).index;
        module().add_spec_instance(symptr, spec_idx);
        return spec_idx;
    }

    /// Find matching function overload according to m_call_args
    Candidate resolve_overload(SymbolPointer symptr, const ast::Identifier& identifier)
    {
        std::vector<Candidate> candidates;
        while (symptr) {
            // resolve nonlocal
            while (symptr->depth() != 0)
                symptr = symptr->ref();

            auto* symmod = symptr.symtab()->module();
            if (symmod == nullptr)
                symmod = &module();
            auto& fn = symmod->get_function(symptr->index());
            const auto& sig_ptr = fn.signature_ptr();
            auto m = match_signature(*sig_ptr);
            candidates.push_back({symmod, symptr->index(), symptr, TypeInfo{sig_ptr}, m});

            symptr = symptr->next();
        }

        auto [found, conflict] = find_best_candidate(candidates);

        if (found && !conflict) {
            if (found->symptr) {
                auto specialized = specialize_function(found->symptr, identifier.source_loc);
                if (specialized) {
                    return {
                        .module = &module(),
                        .index = specialized->index,
                        .type = std::move(specialized->type_info),
                    };
                }
            }
            return *found;
        }

        // format the error message (candidates)
        stringstream o_candidates;
        for (const auto& c : candidates) {
            auto& fn = c.module->get_function(c.index);
            o_candidates << "   " << c.match << "  "
                         << fn.signature() << endl;
        }
        stringstream o_ftype;
        for (const auto& arg : m_call_args) {
            if (&arg != &m_call_args.front())
                o_ftype << ' ';
            o_ftype << arg.type_info;
        }
        if (!m_call_args.empty())
            o_ftype << " -> ";
        if (m_call_ret)
            o_ftype << m_call_ret;
        else
            o_ftype << "Void";
        if (conflict) {
            // ERROR found multiple matching functions
            throw FunctionConflict(identifier.name, o_ftype.str(), o_candidates.str(), identifier.source_loc);
        } else {
            // ERROR couldn't find matching function for `args`
            throw FunctionNotFound(identifier.name, o_ftype.str(), o_candidates.str(), identifier.source_loc);
        }
    }

    // Consume params from `orig_signature` according to `m_call_args`, creating new signature
    std::shared_ptr<Signature>consume_params_from_call_args(const Signature& orig_signature, ast::Call& v) const
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
            if (res->params.front() != arg.type_info) {
                throw UnexpectedArgumentType(i, res->params[0], arg.type_info, arg.source_loc);
            }
            // resolve arg if it's a type var and the signature has a known type in its place
            if (arg.type_info.is_generic() && !res->params.front().is_generic()) {
                specialize_arg(arg.type_info, res->params.front(),
                        m_function.signature().type_args,  // current function, not the called one
                        [i, &arg](const TypeInfo& exp, const TypeInfo& got) {
                            throw UnexpectedArgumentType(i+1, exp, got, arg.source_loc);
                        });
            }
            // consume next param
            ++ v.partial_args;
            if (v.wrapped_execs != 0 && !res->has_closure())
                v.wrapped_execs = 1;
            res->params.erase(res->params.begin());
        }
        return res;
    }

    /// \returns total MatchScore of all parameters and return value, or mismatch
    /// Partial match is possible when the signature has less parameters than call args.
    MatchScore match_signature(const Signature& signature) const
    {
        Signature sig = signature;  // a copy to work on (modified below)
        MatchScore res;
        for (const auto& arg : m_call_args) {
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
            if (!m || m.is_coerce())
                return MatchScore::mismatch();
            res += m;
            // consume next param
            sig.params.erase(sig.params.begin());
        }
        // check return type
        if (m_call_ret) {
            auto m = match_type(m_call_ret, sig.return_type);
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

            // check type of next param
            if (prm != arg.type_info) {
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
            assert(var != 0);
            if (res[var - 1].is_unknown()) {
                if (!m_call_ret.is_unknown())
                    res[var - 1] = m_call_ret;
                if (!m_cast_type.is_unknown())
                    res[var - 1] = m_cast_type.effective_type();
                if (m_type_info)
                    res[var - 1] = m_type_info;
            }
        }
        return res;
    }

    Function& m_function;

    TypeInfo m_type_info;   // resolved ast::Type
    TypeInfo m_value_type;  // inferred type of the value
    TypeInfo m_cast_type;   // target type of Cast

    // signature for resolving overloaded functions and templates
    CallArgs m_call_args;  // actual argument types
    TypeInfo m_call_ret;   // expected return type

    Class* m_class = nullptr;
    Instance* m_instance = nullptr;
    bool m_intrinsic = false;
};


void resolve_types(Function& func, const ast::Block& block)
{
    TypeCheckerVisitor visitor {func};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
    if (!func.signature().has_generic_params() && func.signature().return_type.is_unknown())
        func.signature().return_type = ti_void();
}


} // namespace xci::script
