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
                throw DefinitionTypeMismatch(m_type_info, eval_type, dfn.expression->source_loc);

            m_type_info = std::move(eval_type);

            auto idx_in_cls = m_instance->class_().get_index_of_function(psym->ref()->index());
            m_instance->set_function(idx_in_cls, psym.get_scope_index(m_scope), psym);
        }

        Function& fn = dfn.symbol().get_function(m_scope);

        // Check declared type (decl statement)
        if (fn.signature()) {
            TypeInfo declared_type(fn.signature_ptr());
            if (m_type_info && declared_type != m_type_info) {
                throw DeclarationTypeMismatch(declared_type, m_type_info, dfn.expression->source_loc);
            }
            m_type_info = std::move(declared_type);
        } else {
            if (m_type_info.is_callable())
                fn.signature() = m_type_info.signature();
            else {
                fn.signature().return_type = m_type_info;
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
        // `add_type` deduplicates by comparing the TypeInfos. Unknown equals to any type.
        // The second add_type below overwrites the placeholder.
        auto placeholder_index = module().add_type(TypeInfo{v.type_name.name, ti_unknown()});

        m_type_def_index = placeholder_index;
        v.type->apply(*this);
        m_type_def_index = no_index;

        // create new Named type
        auto index = module().add_type(TypeInfo{v.type_name.name, std::move(m_type_info)});
        assert(index == placeholder_index);
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
            if (!m_type_info.signature().params.empty()) {
                throw DefinitionTypeMismatch(m_type_info, v.value.type_info(), v.source_loc);
            }
            declared = m_type_info.signature().return_type;
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
        v.ti = std::move(m_type_info);
        for (auto& item : v.items) {
            m_type_info = {};
            item->apply(*this);
        }
    }

    void visit(ast::StructInit& v) override {
        auto specified = std::move(m_type_info);
        if (!specified.is_unknown() && !specified.is_struct())
            throw StructTypeMismatch(specified, v.source_loc);
        for (auto& item : v.items) {
            // resolve item type
            if (specified) {
                const TypeInfo* specified_item = specified.struct_item_by_name(item.first.name);
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
                throw UnexpectedTypeArg(v.type_args.front()->source_loc);
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
                    throw MissingTypeArg(v.source_loc);
                if (v.type_args_ti.size() > 1)
                    throw UnexpectedTypeArg(v.type_args[1]->source_loc);
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
            case Symbol::Function: {
                // specified type in declaration
                v.ti = std::move(m_type_info);
                break;
            }
            case Symbol::Module:
                v.ti = ti_unknown();
                break;
            case Symbol::Parameter: {
                const auto* ref_scope = m_scope.find_parent_scope(&symtab);
                v.ti = ref_scope->function().parameter(sym.index());
                break;
            }
            case Symbol::Value:
                if (sym.index() == no_index) {
                    m_intrinsic = true;
                    v.ti = ti_int32();
                } else {
                    TypeChecker type_check(std::move(m_type_info));
                    auto inferred = symtab.module()->get_value(sym.index()).type_info();
                    v.ti = type_check.resolve(inferred, v.source_loc);
                }
                break;
            case Symbol::TypeName:
            case Symbol::TypeVar:
                return;
            case Symbol::StructItem:
                v.ti = std::move(m_type_info);
                break;
            case Symbol::Nonlocal:
            case Symbol::Unresolved:
                XCI_UNREACHABLE;
        }
    }

    void visit(ast::Call& v) override {
        // resolve each argument
        for (auto& arg : v.args) {
            m_type_info = {};
            arg->apply(*this);
        }

        // using resolved args, resolve the callable itself
        // (it may use args types for overload resolution)
        m_type_info = {};
        m_intrinsic = false;
        v.callable->apply(*this);
        v.intrinsic = m_intrinsic;
    }

    void visit(ast::OpCall& v) override {
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        TypeInfo expr_type;
        for (auto& item : v.if_then_expr) {
            item.first->apply(*this);
            item.second->apply(*this);
        }

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

        fn.set_signature(m_type_info.signature_ptr());
        fn.set_ast(v.body);

        resolve_decl(scope, v.body);

        v.ti = m_type_info;
    }

    // The cast expression is translated to a call to `cast` method from the Cast class.
    // The inner expression type and the cast type are used to look up the instance of Cast.
    void visit(ast::Cast& v) override {
        // resolve the target type -> m_type_info
        v.type->apply(*this);
        v.to_type = std::move(m_type_info);

        v.expression->apply(*this);
    }

    void visit(ast::TypeName& t) final {
        m_type_info = resolve_type_name(t.symbol);
    }

    void visit(ast::FunctionType& t) final {
        m_type_def_index = no_index;
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
        m_type_def_index = no_index;
        t.elem_type->apply(*this);
        m_type_info = ti_list(std::move(m_type_info));
    }

    void visit(ast::TupleType& t) final {
        m_type_def_index = no_index;
        std::vector<TypeInfo> subtypes;
        for (auto& st : t.subtypes) {
            st->apply(*this);
            subtypes.push_back(std::move(m_type_info));
        }
        m_type_info = TypeInfo{std::move(subtypes)};
    }

    void visit(ast::StructType& t) final {
        auto type_def_index = m_type_def_index;
        m_type_def_index = no_index;
        TypeInfo::StructItems items;
        for (auto& st : t.subtypes) {
            st.type->apply(*this);
            items.emplace_back(st.identifier.name, std::move(m_type_info));
        }
        m_type_info = TypeInfo{std::move(items)};

        Index index = (type_def_index == no_index) ?
                      module().add_type(m_type_info) : type_def_index;
        for (auto& st : t.subtypes) {
            st.identifier.symbol->set_index(index);
        }
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

    Index m_type_def_index = no_index;  // placeholder for module type being defined in TypeDef

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
