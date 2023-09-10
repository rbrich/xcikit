// resolve_decl.cpp created on 2022-07-17 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "resolve_decl.h"
#include <xci/script/typing/TypeChecker.h>
#include <xci/script/Builtin.h>
#include <xci/script/Function.h>
#include <xci/script/Error.h>
#include <xci/compat/macros.h>

#include <range/v3/view/enumerate.hpp>

#include <optional>

namespace xci::script {

using ranges::views::enumerate;


class ResolveDeclVisitor final: public ast::Visitor {
public:
    explicit ResolveDeclVisitor(Scope& scope) : m_scope(scope) {}

    void visit(ast::Definition& dfn) override {
        // Evaluate specified type
        if (dfn.variable.type) {
            dfn.variable.type->apply(*this);
        } else {
            m_type_info = {};
        }

        if (m_class != nullptr) {
            const auto& psym = dfn.symbol();
            m_class->add_function_scope(psym->index());
        }

        if (m_instance != nullptr) {
            // evaluate type according to class and type vars
            const auto& psym = dfn.symbol();
            const auto& cls_fn = psym->ref().get_function(m_scope);
            TypeInfo eval_type {cls_fn.signature_ptr()};
            for (const auto&& [i, t] : m_instance->types() | enumerate) {
                auto var = m_instance->class_().symtab().find_by_index(Symbol::TypeVar, i + 1);
                eval_type.replace_var(var, t);
            }

            // specified type is basically useless here, let's just check
            // it matches evaluated type from class instance
            if (m_type_info && m_type_info != eval_type)
                throw definition_type_mismatch(m_type_info, eval_type, dfn.expression->source_loc);

            m_type_info = std::move(eval_type);

            auto idx_in_cls = m_instance->class_().get_index_of_function(psym->ref()->index());
            m_instance->set_function(idx_in_cls, &module(), psym.get_scope_index(m_scope), psym);
        }

        Function& fn = dfn.symbol().get_function(m_scope);

        // Check declared type (decl statement)
        if (fn.signature()) {
            TypeInfo declared_type(fn.signature_ptr());
            if (m_type_info && declared_type != m_type_info) {
                throw declaration_type_mismatch(declared_type, m_type_info, dfn.expression->source_loc);
            }
            m_type_info = std::move(declared_type);
        } else {
            if (m_type_info.is_callable())
                fn.signature() = m_type_info.ul_signature();
            else {
                fn.signature().set_parameter(ti_void());
                fn.signature().set_return_type(m_type_info);
            }
        }

        // Expression might use the specified type from `m_type_info`
        if (dfn.expression) {
            dfn.expression->definition = &dfn;
            dfn.expression->apply(*this);
        }

        m_type_info = {};
    }

    void visit(ast::Invocation& inv) override {
        inv.expression->apply(*this);
    }

    void visit(ast::Return& ret) override {
        if (!function().signature().return_type.is_unknown())
            m_type_info = function().signature().return_type;
        ret.expression->apply(*this);
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
        auto index = module().add_type(TypeInfo{v.type_name.name, std::move(m_type_info)});
        v.type_name.symbol->set_index(index);
    }

    void visit(ast::TypeAlias& v) override {
        v.type->apply(*this);

        // add the actual type to Module, referenced by symbol
        Index index = module().add_type(std::move(m_type_info));
        v.type_name.symbol->set_index(index);
    }

    void visit(ast::Literal& v) override {
        TypeInfo declared {m_type_info};
        if (m_type_info.is_callable()) {
            if (m_type_info.ul_signature().has_nonvoid_param()) {
                throw definition_type_mismatch(m_type_info, v.value.type_info(), v.source_loc);
            }
            declared = m_type_info.ul_signature().return_type;
        }
        TypeChecker type_check(std::move(declared));
        v.ti = type_check.resolve(v.value.type_info(), v.source_loc);
    }

    void visit(ast::Tuple& v) override {
        v.ti = std::move(m_type_info);
        for (auto& item : v.items) {
            m_type_info = {};
            item->apply(*this);
        }
    }

    void visit(ast::List& v) override {
        auto specified = std::move(m_type_info);
        if (!specified.is_unknown() && !specified.is_list())
            throw list_type_mismatch(specified, v.source_loc);
        for (auto& item : v.items) {
            m_type_info = specified ? specified.elem_type() : TypeInfo{};
            item->apply(*this);
        }
        m_type_info = {};
        if (specified)
            v.ti = std::move(specified);
    }

    void visit(ast::StructInit& v) override {
        auto specified = std::move(m_type_info);
        if (!specified.is_unknown() && !specified.underlying().is_struct())
            throw struct_type_mismatch(specified, v.source_loc);
        for (auto& item : v.items) {
            // resolve item type
            if (specified) {
                const TypeInfo* specified_item = specified.underlying().struct_item_by_name(item.first.name);
                if (specified_item)
                    m_type_info = *specified_item;
            }
            item.second->apply(*this);
            m_type_info = {};
        }
        v.ti = std::move(specified);
    }

    void visit(ast::Reference& v) override {
        assert(v.identifier.symbol);
        const auto& symtab = *v.identifier.symbol.symtab();
        const auto& sym = *v.identifier.symbol;

        if (!v.type_args.empty()) {
            if (sym.type() != Symbol::Function && sym.type() != Symbol::TypeIndex)
                throw unexpected_type_arg(v.type_args.front()->source_loc);
            auto orig_type_info = std::move(m_type_info);
            for (auto& type_arg : v.type_args) {
                type_arg->apply(*this);
                v.type_args_ti.push_back(std::move(m_type_info));
            }
            m_type_info = std::move(orig_type_info);
        }

        switch (sym.type()) {
            case Symbol::Instruction: {
                // the instructions are low-level, untyped - set return type to Unknown
                m_intrinsic = true;
                return;
            }
            case Symbol::TypeIndex: {
                if (v.type_args_ti.empty())
                    throw missing_type_arg(v.source_loc);
                if (v.type_args_ti.size() > 1)
                    throw unexpected_type_arg(v.type_args[1]->source_loc);
                v.ti = std::move(v.type_args_ti.front());
                v.type_args_ti.clear();
                break;
            }
            case Symbol::Class:
            case Symbol::Instance:
                return;
            case Symbol::Method: {
                // find prototype of the function, resolve actual type of T
                const auto& cls_fn = sym.ref().get_function(m_scope);
                v.ti = TypeInfo{cls_fn.signature_ptr()};
                break;
            }
            case Symbol::Function:
            case Symbol::StructItem: {
                // specified type in declaration
                v.ti = std::move(m_type_info);
                break;
            }
            case Symbol::Module:
                if (sym.index() == no_index) {
                    // builtin __module symbol
                    m_intrinsic = true;
                    v.ti = ti_module();
                } else {
                    // actual module name like `builtin` or `std`
                    v.ti = ti_unknown();
                }
                break;
            case Symbol::Parameter: {
                const auto* ref_scope = m_scope.find_parent_scope(&symtab);
                v.ti = ref_scope->function().parameter(sym.index());
                break;
            }
            case Symbol::Value:
                if (sym.index() == no_index) {
                    m_intrinsic = true;
                    v.ti = ti_int();
                } else {
                    TypeChecker type_check(std::move(m_type_info));
                    auto inferred = symtab.module()->get_value(sym.index()).type_info();
                    v.ti = type_check.resolve(inferred, v.source_loc);
                }
                break;
            case Symbol::TypeName:
            case Symbol::TypeVar:
                return;
            case Symbol::Nonlocal:
            case Symbol::Unresolved:
                XCI_UNREACHABLE;
        }
    }

    void visit(ast::Call& v) override {
        // resolve each argument
        if (v.arg) {
            m_type_info = {};
            v.arg->apply(*this);
        }

        // using resolved args, resolve the callable itself
        // (it may use args types for overload resolution)
        m_type_info = {};
        m_intrinsic = false;
        v.callable->apply(*this);
        v.intrinsic = m_intrinsic;
    }

    void visit(ast::OpCall& v) override {
        assert(!v.right_arg);
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        TypeInfo type_info = std::move(m_type_info);
        for (auto& item : v.if_then_expr) {
            m_type_info = {};
            item.first->apply(*this);
            m_type_info = type_info;
            item.second->apply(*this);
        }
        m_type_info = type_info;
        v.else_expr->apply(*this);
    }

    void visit(ast::WithContext& v) override {
        // resolve type of context (StructInit leads to incomplete struct type)
        v.context->apply(*this);
        v.enter_function.apply(*this);
        v.leave_function.apply(*this);
        // resolve type of expression - it's also the type of the whole "with" expression
        v.expression->apply(*this);
    }

    void visit(ast::Function& v) override {
        auto& scope = module().get_scope(v.scope_index);
        Function& fn = scope.function();

        // specified type (left-hand side of '=')
        TypeInfo specified_type = std::move(m_type_info);
        // lambda type (right hand side of '=')
        v.type.apply(*this);
        assert(m_type_info);
        // fill in types from specified function type
        if (specified_type.is_callable() && m_type_info.is_callable()) {
            const auto& spec_sig = specified_type.ul_signature();
            auto& underlying = m_type_info.underlying();
            if (underlying.signature().return_type.is_unknown() && spec_sig.return_type)
                underlying.signature().set_return_type(spec_sig.return_type);
            if (!m_instance && spec_sig.return_type != underlying.signature().return_type)
                throw declaration_type_mismatch(specified_type, m_type_info, v.source_loc);
            auto& param = underlying.signature().param_type;
            const auto& spec = spec_sig.param_type;
            if (param.is_unknown() || param.is_void())
                param = spec;
            if (param.is_tuple() && spec.is_tuple()) {
                for (const auto& [i, sp] : spec.subtypes() | enumerate) {
                    auto& item = param.subtypes()[i];
                    if (item.is_unknown())
                        item = sp;
                }
            }
            if (param.is_struct() && spec.is_tuple()) {
                for (const auto& [i, sp] : spec.subtypes() | enumerate) {
                    auto& par = param.struct_items()[i].second;
                    if (par.is_unknown())
                        par = sp;
                    else {
                        auto m = match_type(par, sp);
                        if (!m)
                            throw definition_param_type_mismatch(1+i, sp, par, v.source_loc);
                    }
                }
            } else {
                // specified param must match now
                auto m = match_type(param, spec);
                if (!m)
                    throw declaration_type_mismatch(spec, param, v.source_loc);
            }
        } else if (!m_instance && specified_type && specified_type != m_type_info.effective_type())
            throw declaration_type_mismatch(specified_type, m_type_info, v.source_loc);

        fn.set_signature(m_type_info.ul_signature_ptr());

        resolve_decl(scope, v.body);

        v.ti = m_type_info;
    }

    // The cast expression is translated to a call to `cast` method from the Cast class.
    // The inner expression type and the cast type are used to look up the instance of Cast.
    void visit(ast::Cast& v) override {
        // resolve the target type -> m_type_info
        v.type->apply(*this);
        v.ti = std::move(m_type_info);

        v.expression->apply(*this);
    }

    void visit(ast::TypeName& t) final {
        m_type_info = resolve_type_name(t.symbol);
    }

    void visit(ast::FunctionType& t) final {
        auto signature = std::make_shared<Signature>();
        if (t.param) {
            if (t.param.type)
                t.param.type->apply(*this);
            else
                m_type_info = {};
            signature->set_parameter(std::move(m_type_info));
        } else {
            signature->set_parameter(ti_void());
        }
        if (t.return_type)
            t.return_type->apply(*this);
        else
            m_type_info = {};
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
            if (st.type)
                st.type->apply(*this);
            items.emplace_back(st.identifier.name, std::move(m_type_info));
        }
        m_type_info = TypeInfo{std::move(items)};
    }

private:
    Module& module() { return m_scope.module(); }
    Function& function() const { return m_scope.function(); }

    TypeInfo resolve_type_name(SymbolPointer symptr) {
        switch (symptr->type()) {
            case Symbol::TypeName:
                return symptr.symtab()->module()->get_type(symptr->index());
            case Symbol::TypeVar:
                return TypeInfo{ TypeInfo::Var(symptr) };
            default:
                return {};
        }
    }

    Scope& m_scope;

    TypeInfo m_type_info;   // resolved ast::Type

    Class* m_class = nullptr;
    Instance* m_instance = nullptr;
    bool m_intrinsic = false;
};


void resolve_decl(Scope& scope, const ast::Block& block)
{
    ResolveDeclVisitor visitor {scope};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
}


} // namespace xci::script
