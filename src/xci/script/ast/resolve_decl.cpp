// resolve_decl.cpp created on 2022-07-17 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "resolve_decl.h"
#include <xci/script/TypeChecker.h>
#include <xci/script/Builtin.h>
#include <xci/script/Function.h>
#include <xci/script/Error.h>
#include <xci/compat/macros.h>

#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/zip.hpp>

#include <sstream>
#include <optional>

namespace xci::script {

using std::stringstream;
using std::endl;

using ranges::views::enumerate;
using ranges::views::zip;


class ResolveDeclVisitor final: public ast::Visitor {
    struct CallArg {
        TypeInfo type_info;
        bool literal_value;
        SourceLocation source_loc;
    };
    using CallArgs = std::vector<CallArg>;

public:
    explicit ResolveDeclVisitor(FunctionScope& scope) : m_scope(scope) {}

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
            for (const auto&& [i, t] : m_instance->types() | enumerate)
                eval_type.replace_var(i + 1, t);

            // specified type is basically useless here, let's just check
            // it matches evaluated type from class instance
            if (m_type_info && m_type_info != eval_type)
                throw DefinitionTypeMismatch(m_type_info, eval_type, dfn.expression->source_loc);

            m_type_info = std::move(eval_type);

            auto idx_in_cls = m_instance->class_().get_index_of_function(psym->ref()->index());
            m_instance->set_function(idx_in_cls, psym->index(), psym);
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

        m_value_type = {};
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
        v.type_info = type_check.resolve(v.value.type_info(), v.source_loc);
        m_value_type = v.type_info;
    }

    void visit(ast::Tuple& v) override {
        v.literal_type = std::move(m_type_info);
        for (auto& item : v.items) {
            item->apply(*this);
        }
    }

    void visit(ast::List& v) override {
        TypeChecker type_check(std::move(m_type_info));
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
        TypeChecker type_check(std::move(m_type_info));
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
                return;
            }
            case Symbol::TypeId: {
                if (!v.type_arg)
                    throw MissingTypeArg(v.source_loc);
                v.type_info = std::move(m_type_info);
                m_value_type = ti_int32();
                break;
            }
            case Symbol::Class:
            case Symbol::Instance:
                return;
            case Symbol::Method: {
                // find prototype of the function, resolve actual type of T
                const auto& symmod = symtab.module() == nullptr ? module() : *symtab.module();
                const auto& cls_fn = symmod.get_scope(sym.ref()->index()).function();
                v.type_info = TypeInfo{cls_fn.signature_ptr()};
                m_value_type = v.type_info;
                break;
            }
            case Symbol::Function:
            case Symbol::NestedFunction: {
                // specified type in definition
                v.type_info = std::move(m_type_info);
                m_value_type = v.type_info;
                break;
            }
            case Symbol::Module:
                v.type_info = TypeInfo{Type::Module};
                m_value_type = v.type_info;
                break;
            case Symbol::Parameter: {
                const auto* ref_scope = m_scope.find_parent_scope(&symtab);
                v.type_info = ref_scope->function().parameter(sym.index());
                m_value_type = v.type_info;
                break;
            }
            case Symbol::Value:
                if (sym.index() == no_index) {
                    m_intrinsic = true;
                    v.type_info = ti_int32();
                } else {
                    TypeChecker type_check(std::move(m_type_info));
                    auto inferred = symtab.module()->get_value(sym.index()).type_info();
                    v.type_info = type_check.resolve(inferred, v.source_loc);
                }
                m_value_type = v.type_info;
                break;
            case Symbol::TypeName:
            case Symbol::TypeVar:
                return;
            case Symbol::StructItem:
                v.type_info = std::move(m_type_info);
                m_value_type = v.type_info;
                break;
            case Symbol::Nonlocal:
            case Symbol::Unresolved:
                UNREACHABLE;
        }
    }

    void visit(ast::Call& v) override {
        TypeChecker type_check(std::move(m_type_info));

        // resolve each argument
        for (auto& arg : v.args) {
            arg->apply(*this);
        }

        // using resolved args, resolve the callable itself
        // (it may use args types for overload resolution)
        m_intrinsic = false;
        v.callable->apply(*this);
        v.callable_type = m_value_type;
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
        v.expression_type = m_value_type.effective_type();
    }

    void visit(ast::Function& v) override {
        if (v.symbol->type() == Symbol::NestedFunction) {
            v.scope_index = m_scope.get_subscope_index(v.symbol->index());
        }
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
        m_value_type = std::move(m_type_info);

        fn.set_signature(m_value_type.signature_ptr());
        fn.set_ast(v.body);

        resolve_decl(scope, v.body);

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
        v.to_type = std::move(m_type_info);

        v.expression->apply(*this);

        m_value_type = v.to_type;
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
                const auto& type_args = function().signature().type_args;
                if (symptr.symtab() == &function().symtab()
                    && symptr->index() <= type_args.size())
                    return type_args[symptr->index() - 1];
                return TypeInfo{ TypeInfo::Var(symptr->index()) };
            }
            default:
                return {};
        }
    }

    FunctionScope& m_scope;

    TypeInfo m_type_info;   // resolved ast::Type
    TypeInfo m_value_type;  // inferred type of the value

    Index m_type_def_index = no_index;  // placeholder for module type being defined in TypeDef

    Class* m_class = nullptr;
    Instance* m_instance = nullptr;
    bool m_intrinsic = false;
};


void resolve_decl(FunctionScope& scope, const ast::Block& block)
{
    ResolveDeclVisitor visitor {scope};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
}


} // namespace xci::script
