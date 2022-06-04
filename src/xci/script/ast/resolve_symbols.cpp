// resolve_symbols.cpp created on 2019-06-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "resolve_symbols.h"
#include <xci/script/Function.h>
#include <xci/script/Module.h>
#include <xci/script/Builtin.h>
#include <xci/script/Error.h>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/algorithm/any_of.hpp>
#include <vector>
#include <set>
#include <sstream>

namespace xci::script {

using std::make_unique;
using ranges::views::enumerate;


class SymbolResolverVisitor final: public ast::Visitor {
public:
    explicit SymbolResolverVisitor(Function& func) : m_function(func) {}

    void visit(ast::Definition& dfn) override {
        // check for name collision
        const auto& name = dfn.variable.identifier.name;
        auto symptr = symtab().find_by_name(name);

        // allow overloading in some cases
        // * must not have forward declaration
        // * must be a plain function (not method)
        // * must have explicitly specified type
        if (!symptr
        || (!m_class && !m_instance && dfn.variable.type && symptr->is_defined() && symptr->type() == Symbol::Function))
        {
            // not found or undefined -> add new function, symbol
            SymbolTable& fn_symtab = symtab().add_child(name);
            Function fn {module(), fn_symtab};
            auto fn_id = module().add_function(std::move(fn));
            assert(symtab().module() == &module());
            auto new_symptr = symtab().add({name, Symbol::Function, fn_id.index});

            // Overloaded: function's next = the preexisting function
            if (symptr)
                new_symptr->set_next(symptr);

            symptr = new_symptr;
        } else {
            // Allow redefinition only if we're defining plain function, not a method
            if (symptr->is_defined()
            || (!symptr->is_defined() && !dfn.expression)  // multiple forward declarations
            || (!m_class && !m_instance && symptr->type() == Symbol::Method))
                throw RedefinedName(name, dfn.variable.identifier.source_loc);
        }

        if (m_instance) {
            // resolve symbol with the class
            auto ref = m_instance->class_().symtab().find_by_name(name);
            if (!ref)
                throw FunctionNotFoundInClass(name, m_instance->class_().name());
            symptr->set_ref(ref);
        }

        dfn.variable.identifier.symbol = symptr;
        symptr->set_callable(true);
        if (dfn.variable.type)
            dfn.variable.type->apply(*this);
        if (dfn.expression) {
            symptr->set_defined(true);
            dfn.expression->definition = &dfn;
            dfn.expression->apply(*this);
        }

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
            throw RedefinedName(v.class_name.name, v.class_name.source_loc);

        // add child symbol table for the class
        SymbolTable& cls_symtab = symtab().add_child(v.class_name.name);
        for (auto&& [i, type_var] : v.type_vars | enumerate)
            cls_symtab.add({type_var.name, Symbol::TypeVar, Index(i + 1)});

        // add new class to the module
        Class cls {cls_symtab};
        v.index = module().add_class(std::move(cls)).index;
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
            throw UndefinedTypeName(v.class_name.name, v.class_name.source_loc);

        // find next instance of the class (if any)
        auto next = resolve_symbol_of_type(v.class_name.name, Symbol::Instance);

        // create symbol for the instance
        v.class_name.symbol = symtab().add({sym_class, Symbol::Instance});
        v.class_name.symbol->set_next(next);

        // add child symbol table for the instance
        SymbolTable& inst_symtab = symtab().add_child(v.class_name.name);
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
        inst_symtab.set_name(fmt::format("{} ({})",
                v.class_name.name, inst_names.str()));

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
            throw RedefinedName(v.type_name.name, v.type_name.source_loc);

        // resolve the type
        v.type->apply(*this);

        // add new type to symbol table
        v.type_name.symbol = symtab().add({v.type_name.name, Symbol::TypeName, no_index});
    }

    void visit(ast::TypeAlias& v) override {
        // check for name collision
        if (symtab().find_by_name(v.type_name.name))
            throw RedefinedName(v.type_name.name, v.type_name.source_loc);

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
        for (auto& item : v.items) {
            item->apply(*this);
        }
    }

    void visit(ast::StructInit& v) override {
        for (auto& item : v.items) {
            item.second->apply(*this);
        }
    }

    void visit(ast::Reference& v) override {
        auto& symptr = v.identifier.symbol;
        symptr = resolve_symbol(v.identifier.name);
        if (!symptr)
            throw UndefinedName(v.identifier.name, v.source_loc);
        if (v.type_arg)
            v.type_arg->apply(*this);
        if (symptr->type() == Symbol::Method) {
            // if the reference points to a class function, find the nearest
            // instance of the class
            const auto& class_name = symptr.get_class().name();
            v.chain = resolve_symbol_of_type(class_name, Symbol::Instance);
        }
    }

    void visit(ast::Call& v) override {
        v.callable->apply(*this);
        for (auto& arg : v.args) {
            arg->apply(*this);
        }
    }

    void visit(ast::OpCall& v) override {
        assert(!v.right_tmp);
        v.callable = make_unique<ast::Reference>(ast::Identifier{builtin::op_to_function_name(v.op.op), v.source_loc});
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
        v.enter_function = ast::Reference{ast::Identifier{"enter"}};
        v.enter_function.source_loc = v.source_loc;
        v.enter_function.apply(*this);
        v.leave_function = ast::Reference{ast::Identifier{"leave"}};
        v.leave_function.source_loc = v.source_loc;
        v.leave_function.apply(*this);
    }

    void visit(ast::Function& v) override {
        if (v.definition != nullptr) {
            // use Definition's symtab and function
            v.index = v.definition->symbol()->index();
        } else {
            // add new symbol table for the function
            std::string name = "<lambda>";
            if (v.type.params.empty())
                name = "<block>";
            SymbolTable& fn_symtab = symtab().add_child(name);
            Function fn {module(), fn_symtab};
            v.index = module().add_function(std::move(fn)).index;
        }
        Function& fn = module().get_function(v.index);

        v.body.symtab = &fn.symtab();
        m_symtab = v.body.symtab;

        // resolve TypeNames and composite types to symbols
        // (in both parameters and result)
        v.type.apply(*this);

        m_symtab = v.body.symtab->parent();

        // resolve body
        resolve_symbols(fn, v.body);
    }

    void visit(ast::Cast& v) override {
        v.expression->apply(*this);
        v.type->apply(*this);
        v.cast_function = make_unique<ast::Reference>(ast::Identifier{"cast"});
        v.cast_function->source_loc = v.source_loc;
        v.cast_function->apply(*this);
    }

    void visit(ast::TypeName& t) final {
        assert(!t.name.empty());  // can't occur in parsed code
        if (t.name.empty() || t.name[0] == '$') {
            // anonymous generic type
            t.symbol = allocate_type_var(t.name);
            return;
        }
        t.symbol = resolve_symbol(t.name);
        if (!t.symbol)
            throw UndefinedTypeName(t.name, t.source_loc);
    }

    void visit(ast::FunctionType& t) final {
        load_type_params(t.type_params);
        /*
        for (auto& tc : t.context) {
            tc.type_class.apply(*this);
            symtab().add({tc.type_name.name, Symbol::TypeVar, ++type_idx});
        }*/
        Index par_idx = 0;
        for (auto& p : t.params) {
            if (!p.type) {
                // '$T' is internal prefix for untyped function args
                p.type = std::make_unique<ast::TypeName>("$T" + p.identifier.name);
            }
            p.type->apply(*this);
            if (!p.identifier.name.empty())
                p.identifier.symbol = symtab().add({p.identifier.name, Symbol::Parameter, par_idx++});
        }
        if (!t.result_type && !m_instance)
            t.result_type = std::make_unique<ast::TypeName>("$R");
        if (t.result_type)
            t.result_type->apply(*this);
    }

    void visit(ast::ListType& t) final {
        t.elem_type->apply(*this);
    }

    void visit(ast::TupleType& t) final {
        for (auto& st : t.subtypes)
            st->apply(*this);
    }

    void visit(ast::StructType& t) final {
        for (auto& st : t.subtypes)
            st.type->apply(*this);
    }

private:
    Module& module() { return m_function.module(); }
    SymbolTable& symtab() { return *m_symtab; }

    SymbolPointer allocate_type_var(const std::string& name = "") {
        Index idx = 1;
        auto last_var = symtab().find_last_of(Symbol::TypeVar);
        if (last_var)
            idx = last_var->index() + 1;
        return symtab().add({name, Symbol::TypeVar, idx});
    }

    SymbolPointer resolve_symbol(const std::string& name) {
        // lookup intrinsics in builtin module first
        // (this is just an optimization, the same lookup is repeated below)
        if (name.size() > 3 && name[0] == '_' && name[1] == '_') {
            auto& builtin_mod = module().get_imported_module(0);
            assert(builtin_mod.name() == "builtin");
            auto symptr = builtin_mod.symtab().find_by_name(name);
            if (symptr)
                return symptr;
        }
        // (non)local values and parameters
        {
            // lookup in this and parent scopes
            size_t depth = 0;
            for (auto* p_symtab = &symtab(); p_symtab != nullptr; p_symtab = p_symtab->parent()) {
                if (auto symptr = p_symtab->find_by_name(name); symptr) {
                    if (depth > 0 && symptr->type() != Symbol::Method) {
                        // add Nonlocal symbol
                        Index idx = symtab().count(Symbol::Nonlocal);
                        return symtab().add({symptr, Symbol::Nonlocal, idx, depth});
                    }
                    return symptr;
                }

                if (p_symtab->name() == name && p_symtab->parent() != nullptr) {
                    auto symptr = p_symtab->parent()->find_by_name(name);
                    // self-reference in instance function -> find the Method symbol and ref that instead
                    if (symptr && symptr.symtab()->class_() != nullptr) {
                        return symptr.symtab()->class_()->symtab().parent()->find_last_of(name, Symbol::Method);
                    }
                    // recursion - unwrap the function
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

    SymbolPointer resolve_symbol_of_type(const std::string& name, Symbol::Type type) {
        // lookup in this and parent scopes
        for (auto* p_symtab = &symtab(); p_symtab != nullptr; p_symtab = p_symtab->parent()) {
            auto symptr = p_symtab->find_last_of(name, type);
            if (symptr)
                return symptr;
        }
        // this module
        {
            auto symptr = module().symtab().find_last_of(name, type);
            if (symptr)
                return symptr;
        }
        // imported modules
        for (auto i = Index(module().num_imported_modules() - 1); i != Index(-1); --i) {
            auto symptr = module().get_imported_module(
                    i).symtab().find_last_of(name, type);
            if (symptr)
                return symptr;
        }
        // nowhere
        return {};
    }

    void load_type_params(const std::vector<ast::TypeName>& type_params) {
        Index type_idx = 0;
        std::set<std::string> unique;  // check uniqueness
        for (auto& tp : type_params) {
            if (unique.contains(tp.name))
                throw RedefinedName(tp.name, tp.source_loc);
            unique.insert(tp.name);
            symtab().add({tp.name, Symbol::TypeVar, ++type_idx});
        }
    }

private:
    Function& m_function;
    SymbolTable* m_symtab = &m_function.symtab();
    ast::Class* m_class = nullptr;
    Instance* m_instance = nullptr;
};


void resolve_symbols(Function& func, const ast::Block& block)
{
    SymbolResolverVisitor visitor {func};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
}


} // namespace xci::script
