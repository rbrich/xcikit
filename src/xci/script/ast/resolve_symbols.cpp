// resolve_symbols.cpp created on 2019-06-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "resolve_symbols.h"
#include <xci/script/Function.h>
#include <xci/script/Module.h>
#include <xci/script/Builtin.h>
#include <xci/script/Error.h>
#include <xci/script/dump.h>
#include <range/v3/view/enumerate.hpp>
#include <vector>
#include <set>
#include <sstream>

namespace xci::script {

using ranges::views::enumerate;


class ResolveSymbolsVisitor final: public ast::Visitor {
public:
    explicit ResolveSymbolsVisitor(Scope& scope) : m_scope(scope) {}

    void visit(ast::Definition& dfn) override {
        // check for name collision
        const auto name = dfn.variable.identifier.name;
        auto symptr = symtab().find_by_name(name);

        // allow overloading in some cases
        // * must not have a forward declaration
        // * must be a plain function (not method)
        // * must have explicitly specified type
        if (!symptr
        || (!m_class && !m_instance && dfn.variable.type && symptr->is_defined()
                && symptr->type() == Symbol::Function)
        || (symptr->type() == Symbol::StructItem))
        {
            // Either not found, or overloaded -> add new function, symbol
            symptr = create_function(name).first;
        } else {
            // Allow redefinition only if we're defining plain function, not a method
            if (symptr->is_defined()
            || (!symptr->is_defined() && !dfn.expression)  // multiple forward declarations
            || (!m_class && !m_instance && symptr->type() == Symbol::Method))
                throw redefined_name(name, dfn.variable.identifier.source_loc);
        }

        if (m_instance) {
            // resolve symbol with the class
            auto ref = m_instance->class_().symtab().find_by_name(name);
            if (!ref)
                throw function_not_found_in_class(name, m_instance->class_().name());
            symptr->set_ref(ref);
        }

        dfn.variable.identifier.symbol = symptr;
        symptr->set_callable(true);

        // Switch to symtab of the new function
        auto orig_symtab = m_symtab;
        m_symtab = &symptr.get_function(m_scope).symtab();

        if (dfn.variable.type)
            dfn.variable.type->apply(*this);
        if (dfn.expression) {
            Function& fn = symptr.get_function(m_scope);
            fn.set_ast(*dfn.expression);
            fn.set_expression();
            symptr->set_defined(true);
            dfn.expression->definition = &dfn;
            dfn.expression->apply(*this);
        }

        m_symtab = orig_symtab;

        if (m_class) {
            // export symbol to outer scope
            auto outer_sym = symtab().parent()->add({name, Symbol::Method, m_class->index});
            outer_sym->set_ref(symptr);
            return;
        }
    }

    void visit(ast::Invocation& inv) override {
        inv.expression->apply(*this);
    }

    void visit(ast::Return& ret) override {
        ret.expression->apply(*this);
    }

    void visit(ast::Class& v) override {
        // check for name collision
        if (symtab().find_by_name(v.class_name.name))
            throw redefined_name(v.class_name.name, v.class_name.source_loc);

        // add child symbol table and scope for the class
        SymbolTable& cls_symtab = symtab().add_child(v.class_name.name);
        auto scope_idx = module().add_scope(Scope{module(), no_index, &m_scope});
        m_scope.add_subscope(scope_idx);
        cls_symtab.set_scope(&module().get_scope(scope_idx));
        for (auto&& [i, type_var] : v.type_vars | enumerate)
            cls_symtab.add({type_var.name, Symbol::TypeVar, Index(i + 1)});

        // add new class to the module
        v.index = module().add_class(Class{cls_symtab}).index;
        v.symtab = &cls_symtab;

        m_class = &v;
        m_symtab = &cls_symtab;

        for (auto& dfn : v.defs)
            dfn.apply(*this);

        m_symtab = cls_symtab.parent();
        m_class = nullptr;

        // add new symbol
        v.class_name.symbol = symtab().add({v.class_name.name, Symbol::Class, v.index});
    }

    void visit(ast::Instance& v) override {
        // lookup class
        auto sym_class = resolve_symbol_of_type(v.class_name.name, Symbol::Class);
        if (!sym_class)
            throw undefined_type_name(v.class_name.name, v.class_name.source_loc);

        // create symbol for the instance
        v.class_name.symbol = symtab().add({sym_class, Symbol::Instance});

        // add child symbol table and scope for the instance
        SymbolTable& inst_symtab = symtab().add_child(v.class_name.name);
        auto scope_idx = module().add_scope(Scope{module(), no_index, &m_scope});
        m_scope.add_subscope(scope_idx);
        inst_symtab.set_scope(&module().get_scope(scope_idx));
        m_symtab = &inst_symtab;

        // generic instance - add symbols for type params
        load_type_params(v.type_params);

        // resolve type_inst
        std::stringstream inst_names;
        for (auto& t : v.type_inst) {
            t->apply(*this);
            if (t.get() != v.type_inst.front().get())  // not first
                inst_names << ' ';
            inst_names << *t;
        }
        const auto inst_name = fmt::format("{} ({})", v.class_name.name, inst_names.str());
        inst_symtab.set_name(intern(inst_name));

        // add new instance to the module
        Instance inst {sym_class.get_class(), inst_symtab};
        m_instance = &inst;

        for (auto& dfn : v.defs)
            dfn.apply(*this);

        m_instance = nullptr;
        m_symtab = inst_symtab.parent();
        v.index = module().add_instance(std::move(inst)).index;
        v.symtab = &inst_symtab;
        v.class_name.symbol->set_index(v.index);
    }

    void visit(ast::TypeDef& v) override {
        // check for name collision
        if (symtab().find_by_name(v.type_name.name))
            throw redefined_name(v.type_name.name, v.type_name.source_loc);

        // resolve the type
        v.type->apply(*this);

        // add new type to symbol table
        v.type_name.symbol = symtab().add({v.type_name.name, Symbol::TypeName, no_index});
    }

    void visit(ast::TypeAlias& v) override {
        // check for name collision
        if (symtab().find_by_name(v.type_name.name))
            throw redefined_name(v.type_name.name, v.type_name.source_loc);

        // resolve the type
        v.type->apply(*this);

        // add new type to symbol table
        v.type_name.symbol = symtab().add({v.type_name.name, Symbol::TypeName, no_index});
    }

    void visit(ast::Literal&) override {}

    void visit(ast::Tuple& v) override {
        for (auto& item : v.items) {
            item->apply(*this);
        }
    }

    void visit(ast::List& v) override {
        if (v.items.empty()) {
            // Create implicit type var for empty list literal: `[]`
            v.ti = ti_list(TypeInfo(create_implicit_type_var(intern("$L"))));
        } else for (auto& item : v.items) {
            item->apply(*this);
        }
    }

    void visit(ast::StructInit& v) override {
        std::set<NameId> keys;
        for (auto& item : v.items) {
            // check the key is not duplicate
            auto [_, ok] = keys.insert(item.first.name);
            if (!ok)
                throw struct_duplicate_key(item.first.name, item.first.source_loc);

            item.second->apply(*this);
            item.first.symbol = add_struct_item(item.first.name, no_index);
        }
    }

    void visit(ast::Reference& v) override {
        auto& symptr = v.identifier.symbol;
        symptr = resolve_symbol(v.identifier.name);
        if (!symptr)
            throw undefined_name(v.identifier.name, v.source_loc);
        for (auto& type_arg : v.type_args)
            type_arg->apply(*this);
        if (symptr->type() == Symbol::Method) {
            if (v.definition) {
                // find all instances of the class
                const auto& class_name = symptr.get_class().name();
                v.sym_list.emplace_back(symptr);
                auto instances = find_all_symbols_of_type(class_name, Symbol::Instance);
                v.sym_list.insert(v.sym_list.end(), instances.begin(), instances.end());
            } else {
                // if the reference points to a method, find all instances of the class
                // sym_list will contain each Method symbol followed by all found Instances
                // (there can be multiple Methods from different classes sharing the same name)
                auto method_syms = find_all_symbols_of_type(v.identifier.name, Symbol::Method);
                assert(std::find(method_syms.begin(), method_syms.end(), symptr) != method_syms.end());
                for (auto method_sym : method_syms) {
                    v.sym_list.emplace_back(method_sym);
                    const auto& class_name = method_sym.get_class().name();
                    auto instances = find_all_symbols_of_type(class_name, Symbol::Instance);
                    v.sym_list.insert(v.sym_list.end(), instances.begin(), instances.end());
                }
            }
        }
        if (symptr->type() == Symbol::Function || symptr->type() == Symbol::StructItem) {
            // find all visible function overloads (in the nearest scope)
            v.sym_list = find_function_overloads(v.identifier.name);
            // find all StructItem symbols, in any modules
            auto struct_syms = find_all_symbols_of_type(v.identifier.name, Symbol::StructItem);
            if (!struct_syms.empty()) {
                // Always insert only a single StructItem symbol.
                v.sym_list.emplace_back(struct_syms.front());
            }
        }
        if (symptr->type() == Symbol::Module) {
            // add module to overload set (only if it's actual module symbol, not builtin __module)
            if (symptr->index() != no_index)
                v.sym_list.emplace_back(symptr);
        }
    }

    void visit(ast::Call& v) override {
        if (v.arg)
            v.arg->apply(*this);
        v.callable->apply(*this);
    }

    void visit(ast::OpCall& v) override {
        assert(!v.right_tmp);
        const auto fn_name = intern(builtin::op_to_function_name(v.op.op));
        v.callable = std::make_unique<ast::Reference>(ast::Identifier{fn_name, v.source_loc});
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        for (auto& item : v.if_then_expr) {
            item.first->apply(*this);
            item.second->apply(*this);
        }
        v.else_expr->apply(*this);
    }

    void visit(ast::WithContext& v) override {
        v.context->apply(*this);
        v.expression->apply(*this);
        v.enter_function = ast::Reference{ast::Identifier{intern("enter")}};
        v.enter_function.source_loc = v.source_loc;
        v.enter_function.apply(*this);
        v.leave_function = ast::Reference{ast::Identifier{intern("leave")}};
        v.leave_function.source_loc = v.source_loc;
        v.leave_function.apply(*this);
    }

    void visit(ast::Function& v) override {
        if (v.definition != nullptr) {
            // use Definition's symtab and function
            v.symbol = v.definition->symbol();
            v.scope_index = v.symbol.get_scope_index(m_scope);
            //v.scope_index = symtab().scope()->get_subscope_index(v.symbol->index());
        } else {
            // add new symbol table for the function
            auto num = symtab().count(Symbol::Function);
            std::string name;
            if (!v.type.param)
                name = fmt::format("<block_{}>", num);
            else
                name = fmt::format("<lambda_{}>", num);
            std::tie(v.symbol, v.scope_index) = create_function(intern(name));
        }
        auto& scope = module().get_scope(v.scope_index);
        Function& fn = scope.function();
        fn.set_ast(v.body);
        fn.set_expression(false);

        v.body.symtab = &fn.symtab();
        m_symtab = v.body.symtab;

        // resolve TypeNames and composite types to symbols
        // (in both parameters and result)
        v.type.apply(*this);

        m_symtab = v.body.symtab->parent();

        // resolve body
        resolve_symbols(scope, v.body);
    }

    void visit(ast::Cast& v) override {
        v.expression->apply(*this);
        v.type->apply(*this);
        const auto fn_name = intern(v.is_init ? "init" : "cast");
        v.cast_function = std::make_unique<ast::Reference>(ast::Identifier{fn_name});
        v.cast_function->source_loc = v.source_loc;
        v.cast_function->apply(*this);
    }

    void visit(ast::TypeName& t) final {
        assert(!t.name.empty());  // can't occur in parsed code
        if (t.name.view()[0] == '$') {
            // anonymous generic type
            t.symbol = create_implicit_type_var(t.name);
            return;
        }
        t.symbol = resolve_symbol(t.name);
        if (!t.symbol)
            throw undefined_type_name(t.name, t.source_loc);
    }

    void visit(ast::FunctionType& t) final {
        load_type_params(t.type_params);
        /*
        for (auto& tc : t.context) {
            tc.type_class.apply(*this);
            symtab().add({tc.type_name.name, Symbol::TypeVar, ++type_idx});
        }*/
        if (t.param) {
            m_parameter = true;
            auto& p = t.param;
            if (!p.type) {
                // '$P' is internal prefix for untyped function args
                const auto type_name = intern(fmt::format("$P{}", p.identifier.name));
                p.type = std::make_unique<ast::TypeName>(type_name);
            }
            p.type->apply(*this);
            // Special case for unnamed struct parameter - create Parameter symbols for subtypes
            auto* ps = dynamic_cast<ast::StructType*>(p.type.get());
            if (p.identifier.name.empty() && ps) {
                Index idx = 0;
                for (auto& st : ps->subtypes)
                    st.identifier.symbol = symtab().add({st.identifier.name, Symbol::Parameter, idx++});
            }
            if (!p.identifier.name.empty())
                p.identifier.symbol = symtab().add({p.identifier.name, Symbol::Parameter, no_index});
            m_parameter = false;
        }
        if (!t.return_type && !m_instance)
            t.return_type = std::make_unique<ast::TypeName>(intern("$R"));
        if (t.return_type)
            t.return_type->apply(*this);
    }

    void visit(ast::ListType& t) final {
        t.elem_type->apply(*this);
    }

    void visit(ast::TupleType& t) final {
        for (auto& st : t.subtypes)
            st->apply(*this);
    }

    void visit(ast::StructType& t) final {
        std::set<NameId> keys;
        for (auto& st : t.subtypes) {
            const NameId name = st.identifier.name;

            // check the key is not duplicate
            auto [_, ok] = keys.insert(name);
            if (!ok)
                throw struct_duplicate_key(name, st.identifier.source_loc);

            if (st.type) {
                st.type->apply(*this);
            } else if (m_parameter) {
                st.type = std::make_unique<ast::TypeName>(intern(fmt::format("$T{}", name)));
                st.type->apply(*this);
            }
            st.identifier.symbol = add_struct_item(name, no_index);
        }
    }

private:
    Module& module() { return m_scope.module(); }
    Function& function() { return m_scope.function(); }
    SymbolTable& symtab() { return *m_symtab; }

    std::pair<SymbolPointer, Index> create_function(NameId name) {
        SymbolTable& fn_symtab = symtab().add_child(name);
        auto fn_idx = module().add_function(Function{module(), fn_symtab}).index;
        auto scope_idx = module().add_scope(Scope{module(), fn_idx, symtab().scope()});
        auto subscope_i = symtab().scope()->add_subscope(scope_idx);
        assert(symtab().module() == &module());
        auto symptr = symtab().add({name, Symbol::Function, subscope_i});
        return {symptr, scope_idx};
    }

    SymbolPointer add_struct_item(NameId name, Index idx) {
        auto& symtab = module().symtab();
        // Deduplicate StructItem symbols - they don't carry any information
        // other than the name may be a struct member
        auto sym_ptr = symtab.find_last_of(name, Symbol::StructItem);
        if (sym_ptr) {
            assert(sym_ptr.symtab() == &symtab);
            return sym_ptr;
        }
        return symtab.add({name, Symbol::StructItem, idx});
    }

    SymbolPointer create_implicit_type_var(NameId name) {
        Index idx = 1;
        auto last_var = symtab().find_last_of(Symbol::TypeVar);
        if (last_var)
            idx = last_var->index() + 1;
        auto symptr = symtab().add({name, Symbol::TypeVar, idx});
        symptr->set_implicit();
        return symptr;
    }

    SymbolPointer resolve_symbol(NameId name) {
        // lookup intrinsics in builtin module first
        // (this is just an optimization, the same lookup is repeated below)
        auto name_view = name.view();
        if (name_view.size() > 3 && name_view[0] == '_' && name_view[1] == '_') {
            auto& builtin_mod = module().get_imported_module(0);
            assert(builtin_mod.name().view() == "builtin");
            auto symptr = builtin_mod.symtab().find_by_name(name);
            if (symptr)
                return symptr;
        }
        // local functions and parameters
        {
            // lookup in this and parent scopes
            unsigned depth = 0;
            for (auto* p_symtab = &symtab(); p_symtab != nullptr; p_symtab = p_symtab->parent()) {
                if (auto symptr = p_symtab->find_by_name(name); symptr)
                    return symptr;

                if (p_symtab->name() == name && p_symtab->parent() != nullptr) {
                    auto symptr = p_symtab->parent()->find_by_name(name);
                    // self-reference in instance function -> find the Method symbol and ref that instead
                    if (symptr && symptr.symtab()->class_() != nullptr) {
                        return symptr.symtab()->class_()->symtab().parent()->find_last_of(name, Symbol::Method);
                    }
                    // recursion - unwrap the function
                    assert(symptr->type() == Symbol::Function);
                    return symtab().add({symptr, Symbol::Function, no_index, depth + 1});
                }

                depth ++;
            }
        }
        // this module
        {
            auto symptr = module().symtab().find_by_name(name);
            if (symptr)
                return symptr;
        }
        // imported modules
        for (auto i = Index(module().num_imported_modules() - 1); i != Index(-1); --i) {
            auto symptr = module().get_imported_module(i).symtab().find_by_name(name);
            if (symptr)
                return symptr;
        }
        // nowhere
        return {};
    }

    SymbolPointer resolve_symbol_of_type(NameId name, Symbol::Type type) {
        // lookup in this and parent scopes (including this module scope)
        for (auto* p_symtab = &symtab(); p_symtab != nullptr; p_symtab = p_symtab->parent()) {
            auto symptr = p_symtab->find_last_of(name, type);
            if (symptr)
                return symptr;
        }
        // imported modules
        for (auto i = Index(module().num_imported_modules() - 1); i != Index(-1); --i) {
            auto symptr = module().get_imported_module(i).symtab().find_last_of(name, type);
            if (symptr)
                return symptr;
        }
        // nowhere
        return {};
    }

    SymbolPointerList find_all_symbols_of_type(NameId name, Symbol::Type type) {
        SymbolPointerList res;
        // lookup in this module
        {
            auto sym_list = module().symtab().filter(name, type);
            res.insert(res.end(), sym_list.begin(), sym_list.end());
        }
        // imported modules
        for (auto i = Index(module().num_imported_modules() - 1); i != Index(-1); --i) {
            auto sym_list = module().get_imported_module(i).symtab().filter(name, type);
            res.insert(res.end(), sym_list.begin(), sym_list.end());
        }
        return res;
    }

    SymbolPointerList find_function_overloads(NameId name) {
        // lookup in this and parent scopes (including this module scope)
        for (auto* p_symtab = &symtab(); p_symtab != nullptr; p_symtab = p_symtab->parent()) {
            auto sym_list = p_symtab->filter(name, Symbol::Function);
            if (!sym_list.empty())
                return sym_list;
        }
        // imported modules
        for (auto i = Index(module().num_imported_modules() - 1); i != Index(-1); --i) {
            auto sym_list = module().get_imported_module(i).symtab().filter(name, Symbol::Function);
            if (!sym_list.empty())
                return sym_list;
        }
        // nowhere
        return {};
    }

    void load_type_params(const std::vector<ast::TypeName>& type_params) {
        Index type_idx = 0;
        std::set<NameId> unique;  // check uniqueness
        for (auto& tp : type_params) {
            if (unique.contains(tp.name))
                throw redefined_name(tp.name, tp.source_loc);
            unique.insert(tp.name);
            symtab().add({tp.name, Symbol::TypeVar, ++type_idx});
        }
    }

private:
    Scope& m_scope;
    SymbolTable* m_symtab = &function().symtab();
    ast::Class* m_class = nullptr;
    Instance* m_instance = nullptr;
    bool m_parameter = false;  // are we resolving parameter (FunctionType)?
};


void resolve_symbols(Scope& scope, const ast::Block& block)
{
    ResolveSymbolsVisitor visitor {scope};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
}


} // namespace xci::script
