// resolve_nonlocals.cpp created on 2020-01-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "resolve_nonlocals.h"
#include <xci/script/Function.h>
#include <xci/script/Module.h>

namespace xci::script {


static void resolve_nonlocals_for_scope(FunctionScope& scope, const ast::Block& block);


void resolve_nonlocals_in_symtab(FunctionScope& scope)
{
    auto& fn = scope.function();
    if (fn.signature().has_generic_nonlocals()) {
        // Generic nonlocals mean the function itself was not generic,
        // but it's nonlocals reference parameters of parent generic function
        // or a generic function that will need a closure.
        //
        // Until now, the function wouldn't need to be specialized.
        // Mark it as generic (not to be compiled).
        fn.set_compile(false);
    }

    auto& scope_nonlocals = scope.nonlocals();
    const auto& sig_nonlocals = fn.signature().nonlocals;

    if (!scope_nonlocals.empty()) {
        // already resolved
        assert(scope_nonlocals.size() == sig_nonlocals.size());
        return;
    }
    if (sig_nonlocals.empty()) {
        // no nonlocals
        return;
    }

    for (const auto& sym : fn.symtab()) {
        if (sym.type() == Symbol::Nonlocal) {
            assert(sym.depth() == 1);
            switch (sym.ref()->type()) {
                case Symbol::Parameter: {
                    auto* parent_scope = scope.parent();
                    auto& parent_fn = parent_scope->function();
                    const TypeInfo& ti = parent_fn.parameter(sym.ref()->index());
                    scope.add_nonlocal(sym.index(), ti);
                    break;
                }
                case Symbol::Function: {
                    auto& ref_mod = *sym.ref().symtab()->module();
                    auto& ref_scope = ref_mod.get_scope(sym.ref()->index());
//                    auto& ref_fn = ref_scope.function();
//                    if (ref_fn.is_generic() && !ref_fn.has_nonlocals_resolved()) {
//                        ref_fn.set_nonlocals_resolved();
//                        resolve_nonlocals_for_scope(ref_scope, ref_fn.ast());
//                    }
                    resolve_nonlocals_in_symtab(ref_scope);
                    scope.add_nonlocal(sym.index(), TypeInfo{ref_scope.function().signature_ptr()});
                    break;
                }
                case Symbol::NestedFunction: {
                    auto* parent_scope = scope.parent();
                    auto& ref_scope = parent_scope->get_subscope(sym.ref()->index());
//                    auto& ref_fn = ref_scope.function();
//                    if (ref_fn.is_generic() && !ref_fn.has_nonlocals_resolved()) {
//                        ref_fn.set_nonlocals_resolved();
//                        resolve_nonlocals_for_scope(ref_scope, ref_fn.ast());
//                    }
                    resolve_nonlocals_in_symtab(ref_scope);
                    scope.add_nonlocal(sym.index(), TypeInfo{ref_scope.function().signature_ptr()});
                    break;
                }
                default:
                    break;
            }
        }
//        if (sym.type() == Symbol::Function && sym.ref() && sym.ref()->type() == Symbol::Function) {
//            // unwrap function (self-)reference
//            sym.set_index(sym.ref()->index());
//        }
    }
}


class ResolveNonlocalsVisitor final: public ast::VisitorExclTypes {
public:
    using VisitorExclTypes::visit;

    explicit ResolveNonlocalsVisitor(FunctionScope& scope)
            : m_scope(scope) {}

    void visit(ast::Definition& dfn) override {
        if (dfn.expression)
            dfn.expression->apply(*this);
    }

    void visit(ast::Invocation& inv) override {
        inv.expression->apply(*this);
    }

    void visit(ast::Return& ret) override {
        ret.expression->apply(*this);
    }

    void visit(ast::Class&) override {}
    void visit(ast::Instance&) override {}
    void visit(ast::Literal&) override {}
    void visit(ast::Cast& v) override {
        v.expression->apply(*this);
    }

    void visit(ast::Parenthesized& v) override {
        v.expression->apply(*this);
    }

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
        assert(v.identifier.symbol);
        const auto& symtab = *v.identifier.symbol.symtab();
        const auto& symptr = v.identifier.symbol;
        switch (symptr->type()) {
            case Symbol::Parameter: {
                const auto* ref_scope = m_scope.find_parent_scope(&symtab);
                const TypeInfo& ti = ref_scope->function().parameter(symptr->index());
                auto nl_symptr = add_nonlocal_symbol(m_scope, symptr, v.identifier.name, ti);
                if (nl_symptr)
                    v.identifier.symbol = nl_symptr;
                break;
            }
            case Symbol::Function:
            case Symbol::NestedFunction: {
                assert(v.index != no_index);
//                if (v.index == no_index) {
//                    assert(symtab.module() != nullptr);
//                    v.module = symtab.module();
//                    v.index = symptr.get_scope_index(m_scope);
//                }
                auto& ref_scope = v.module->get_scope(v.index);
                auto& ref_fn = ref_scope.function();
                if (ref_fn.is_generic() && !ref_fn.has_nonlocals_resolved()) {
                    ref_fn.set_nonlocals_resolved();
                    resolve_nonlocals_for_scope(ref_scope, ref_fn.ast());
                } else {
                    resolve_nonlocals_in_symtab(ref_scope);
                }
                if (v.module != &module()) {
                    // Referenced function is from another module -> we're done
                    break;
                }
                // partial calls
                if (!function().partial().empty()) {
                    function().symtab().set_name(v.identifier.name + "/partial");
                    if (ref_scope.has_nonlocals()) {
                        Index nl_idx = function().symtab().count(Symbol::Nonlocal);
                        auto nl_sym = function().symtab().add({
                                v.identifier.symbol,
                                Symbol::Nonlocal,
                                nl_idx,
                                1});
                        m_scope.add_nonlocal(nl_idx, TypeInfo{ref_fn.signature_ptr()}, v.index);
                        v.identifier.symbol = nl_sym;
                        //v.identifier.symbol->set_callable(true);
                    }
                    break;
                }
                // add Nonlocal symbol
                if (ref_scope.has_nonlocals()) {
                    auto nl_symptr = add_nonlocal_symbol(m_scope, symptr, v.identifier.name,
                                                         TypeInfo{ref_scope.function().signature_ptr()}, v.index);
                    if (nl_symptr)
                        v.identifier.symbol = nl_symptr;
                }
                break;
            }
            default:
                break;
        }
    }

    void visit(ast::Call& v) override {
        for (auto& arg : v.args) {
            arg->apply(*this);
        }

        if (v.partial_index != no_index) {
            auto& scope = module().get_scope(v.partial_index);
            process_subroutine(scope, *v.callable);
        } else {
            v.callable->apply(*this);
        }
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
    }

    void visit(ast::WithContext& v) override {
        v.context->apply(*this);
        v.expression->apply(*this);
    }

    void visit(ast::Function& v) override {
//        auto& scope = module().get_scope(v.scope_index);
//        Function& fn = scope.function();
//        if (fn.is_generic() && fn.has_compile() && !fn.has_nonlocals_resolved()) {
//            fn.set_nonlocals_resolved();
//            resolve_nonlocals_for_scope(scope, fn.ast());
//        }
    }

private:
    Module& module() { return m_scope.module(); }
    Function& function() const { return m_scope.function(); }

    void process_subroutine(FunctionScope& scope, ast::Expression& expression) {
        ResolveNonlocalsVisitor visitor(scope);
        expression.apply(visitor);
    }

    SymbolPointer add_nonlocal_symbol(FunctionScope& scope, SymbolPointer symptr,
                                      const std::string& name, const TypeInfo& ti,
                                      Index fn_scope_idx = no_index)
    {
        auto& my_symtab = scope.function().symtab();
        //auto depth = my_symtab.depth(symptr.symtab());
        if (&my_symtab == symptr.symtab())
            return {};  // depth == 0, not non-local
        if (my_symtab.parent() != symptr.symtab()) {
            // depth > 1, add intermediate non-local to parent
            symptr = add_nonlocal_symbol(*scope.parent(), symptr, name, ti, fn_scope_idx);
        }
        assert(my_symtab.parent() == symptr.symtab());  // depth == 1

        for (auto& sym : my_symtab) {
            if (sym.type() == Symbol::Nonlocal && sym.name() == name) {
                assert(sym.ref() == symptr);
                scope.add_nonlocal(sym.index(), ti, fn_scope_idx);
                return my_symtab.find(sym);
            }
        }

        auto nl_idx = my_symtab.count(Symbol::Nonlocal);
        auto new_symptr = my_symtab.add({symptr, Symbol::Nonlocal, nl_idx, 1});
        new_symptr->set_callable(ti.is_callable());
        scope.add_nonlocal(nl_idx, TypeInfo{ti}, fn_scope_idx);
        return new_symptr;
    }

private:
    FunctionScope& m_scope;
};


static void resolve_nonlocals_for_scope(FunctionScope& scope, const ast::Block& block)
{
    ResolveNonlocalsVisitor visitor {scope};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
    resolve_nonlocals_in_symtab(scope);
}


void resolve_nonlocals(FunctionScope& main, const ast::Block& block)
{
    resolve_nonlocals_for_scope(main, block);

    // Resolve other scopes (not referenced directly via main scope or AST)
    Module& module = main.module();
    for (unsigned i = 0; i != module.num_scopes(); ++i) {
        FunctionScope& scope = module.get_scope(i);
        if (&scope == &main)
            continue;
        Function& fn = scope.function();
        if (fn.has_nonlocals_resolved() || !fn.is_generic() || !fn.has_compile())
            continue; // already resolved
        fn.set_nonlocals_resolved();
        resolve_nonlocals_for_scope(scope, fn.ast());
    }
}


} // namespace xci::script
