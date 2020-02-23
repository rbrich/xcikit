// resolve_symbols.cpp created on 2019-06-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "resolve_symbols.h"
#include <xci/script/Function.h>
#include <xci/script/Module.h>
#include <xci/script/Builtin.h>
#include <xci/script/Error.h>
#include <vector>

namespace xci::script {

using std::make_unique;
using std::string;


class SymbolResolverVisitor final: public ast::Visitor {
public:
    explicit SymbolResolverVisitor(Function& func) : m_function(func) {}

    void visit(ast::Definition& dfn) override {
        // check for name collision
        const auto& name = dfn.variable.identifier.name;
        if (symtab().find_by_name(name))
            throw MultipleDeclarationError(name);

        // add new function, symbol
        SymbolTable& fn_symtab = symtab().add_child(name);
        auto fn = make_unique<Function>(module(), fn_symtab);
        auto idx = module().add_function(move(fn));
        dfn.variable.identifier.symbol = symtab().add({name, Symbol::Function, idx});
        dfn.variable.identifier.symbol->set_callable(true);
        if (dfn.variable.type)
            dfn.variable.type->apply(*this);
        if (dfn.expression) {
            dfn.expression->definition = &dfn;
            dfn.expression->apply(*this);
        }

        if (m_class) {
            // export symbol to outer scope
            auto outer_sym = symtab().parent()->add({name,
                                                     Symbol::Method, m_class->index});
            outer_sym->set_ref(dfn.variable.identifier.symbol);
            return;
        }

        if (m_instance) {
            // resolve symbol with the class
            auto ref = m_instance->class_().symtab().find_by_name(dfn.variable.identifier.name);
            if (!ref)
                throw FunctionNotFoundInClass(dfn.variable.identifier.name, m_instance->class_().name());
            dfn.variable.identifier.symbol->set_ref(ref);
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
            throw MultipleDeclarationError(v.class_name.name);

        // add child symbol table for the class
        SymbolTable& cls_symtab = symtab().add_child(v.class_name.name);
        cls_symtab.add({v.type_var.name, Symbol::TypeVar, 1});

        // add new class to the module
        auto cls = make_unique<Class>(cls_symtab);
        v.index = module().add_class(move(cls));
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
            throw UndefinedTypeName(v.class_name.name);

        // find next instance of the class (if any)
        auto next = resolve_symbol_of_type(v.class_name.name, Symbol::Instance);

        // create symbol for the instance
        v.class_name.symbol = symtab().add({sym_class, Symbol::Instance});
        v.class_name.symbol->set_next(next);

        // resolve type_inst
        v.type_inst->apply(*this);

        // add child symbol table for the instance
        SymbolTable& inst_symtab = symtab().add_child(core::format("{} ({})",
                v.class_name.name, *v.type_inst));
        m_symtab = &inst_symtab;

        // add new instance to the module
        auto& cls = module().get_class(sym_class->index());
        auto inst = make_unique<Instance>(cls, inst_symtab);
        m_instance = inst.get();

        for (auto& dfn : v.defs)
            dfn.apply(*this);

        m_instance = nullptr;
        m_symtab = inst_symtab.parent();
        v.index = module().add_instance(move(inst));
        v.symtab = &inst_symtab;
        v.class_name.symbol->set_index(v.index);
    }

    void visit(ast::Integer& v) override {}
    void visit(ast::Float& v) override {}
    void visit(ast::String& v) override {}

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

    void visit(ast::Reference& v) override {
        auto& symptr = v.identifier.symbol;
        symptr = resolve_symbol(v.identifier.name);
        if (!symptr)
            throw UndefinedName(v.identifier.name, v.source_info);
        if (symptr->type() == Symbol::Method) {
            // if the reference points to a class function, find nearest
            // instance of the class
            auto* symmod = symptr.symtab()->module();
            if (symmod == nullptr)
                symmod = &module();
            auto& class_name = symmod->get_class(symptr->index()).name();
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
        v.callable = make_unique<ast::Reference>(ast::Identifier{builtin::op_to_function_name(v.op.op)});
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        v.cond->apply(*this);
        v.then_expr->apply(*this);
        v.else_expr->apply(*this);
    }

    void visit(ast::Function& v) override {
        if (v.definition != nullptr) {
            // use Definition's symtab and function
            v.index = v.definition->symbol()->index();
        } else {
            // add new symbol table for the function
            SymbolTable& fn_symtab = symtab().add_child("<lambda>");
            auto fn = make_unique<Function>(module(), fn_symtab);
            v.index = module().add_function(move(fn));
        }
        Function& fn = module().get_function(v.index);

        v.body.symtab = &fn.symtab();
        m_symtab = v.body.symtab;

        // resolve TypeNames and composite types to symbols
        // (in both parameters and result)
        v.type.apply(*this);

        m_symtab = v.body.symtab->parent();

        // postpone body compilation
        m_postponed_blocks.push_back({fn, v.body});
    }

    void visit(ast::TypeName& t) final {
        if (t.name.empty())
            //  TypeInfo(Type::Unknown); ?
            throw UndefinedTypeName(t.name);
        t.symbol = resolve_symbol(t.name);
        if (!t.symbol)
            throw UndefinedTypeName(t.name);
    }

    void visit(ast::FunctionType& t) final {
        size_t type_idx = 0;
        for (auto& tc : t.context) {
            tc.type_class.apply(*this);
            symtab().add({tc.type_name.name, Symbol::TypeVar, ++type_idx});
        }
        size_t par_idx = 0;
        for (auto& p : t.params) {
            if (p.type)
                p.type->apply(*this);
            p.identifier.symbol = symtab().add({p.identifier.name, Symbol::Parameter, par_idx++});
        }
        if (t.result_type)
            t.result_type->apply(*this);
    }

    void visit(ast::ListType& t) final {
        t.elem_type->apply(*this);
    }

    struct PostponedBlock {
        Function& func;
        const ast::Block& block;
    };
    const std::vector<PostponedBlock>& postponed_blocks() const { return m_postponed_blocks; }

private:
    Module& module() { return m_function.module(); }
    SymbolTable& symtab() { return *m_symtab; }

    SymbolPointer resolve_symbol(const string& name) {
        // lookup intrinsics in builtin module first
        // (this is just an optimization, the same lookup is repeated below)
        if (name.size() > 3 && name[0] == '_' && name[1] == '_') {
            auto symptr = module().get_imported_module(0).symtab().find_by_name(name);
            if (symptr)
                return symptr;
        }
        // (non)local values and parameters
        {
            // lookup in this and parent scopes
            size_t depth = 0;
            for (auto p_symtab = &symtab(); p_symtab != nullptr; p_symtab = p_symtab->parent()) {
                if (p_symtab->name() == name && p_symtab->parent() != nullptr) {
                    // recursion - unwrap the function
                    auto symptr = p_symtab->parent()->find_by_name(name);
                    return symtab().add({symptr, Symbol::Function, depth + 1});
                }

                auto symptr = p_symtab->find_by_name(name);
                if (symptr) {
                    if (depth > 0 && symptr->type() != Symbol::Method) {
                        // add Nonlocal symbol
                        return symtab().add({symptr, Symbol::Nonlocal, depth});
                    } else {
                        return symptr;
                    }
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
        for (size_t i = 0; i < module().num_imported_modules(); i++) {
            auto symptr = module().get_imported_module(i).symtab().find_by_name(name);
            if (symptr)
                return symptr;
        }
        // nowhere
        return {};
    }

    SymbolPointer resolve_symbol_of_type(const string& name, Symbol::Type type) {
        // lookup in this and parent scopes
        for (auto p_symtab = &symtab(); p_symtab != nullptr; p_symtab = p_symtab->parent()) {
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
        for (size_t i = 0; i < module().num_imported_modules(); i++) {
            auto symptr = module().get_imported_module(
                    i).symtab().find_last_of(name, type);
            if (symptr)
                return symptr;
        }
        // nowhere
        return {};
    }

private:
    std::vector<PostponedBlock> m_postponed_blocks;
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

    // process postponed blocks
    for (const auto& blk : visitor.postponed_blocks()) {
        resolve_symbols(blk.func, blk.block);
    }
}


} // namespace xci::script
