// resolve_nonlocals.cpp created on 2020-01-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "resolve_nonlocals.h"
#include <xci/script/Function.h>
#include <xci/script/Module.h>

namespace xci::script {


// This function contains most of the logic - see description in resolve_nonlocals.h
void resolve_nonlocals_in_symtab(Function& func)
{
    if (func.test_and_set_nonlocals_resolved())
        return;
    auto& nonlocals = func.signature().nonlocals;
    if (nonlocals.empty())
        return;

    auto nonlocals_erased = 0;
    for (auto& sym : func.symtab()) {
        if (sym.type() == Symbol::Nonlocal) {
            sym.set_index(sym.index() - nonlocals_erased);
            if (sym.ref() && sym.ref()->type() == Symbol::Function) {
                // unwrap reference to non-value function
                auto& ref_fn = sym.ref().get_function();
                if (!ref_fn.has_nonlocals()) {
                    nonlocals.erase(nonlocals.begin() + (ptrdiff_t) sym.index());
                    ++ nonlocals_erased;
                    sym = *sym.ref();
                }
            } else if (sym.depth() > 1) {
                // not direct parent -> add intermediate Nonlocal
                auto ti = sym.ref().symtab()->function()->parameter(sym.ref()->index());
                auto* parent_fn = func.symtab().parent()->function();
                auto idx = parent_fn->add_nonlocal(std::move(ti));
                parent_fn->symtab().add({sym.ref(), Symbol::Nonlocal, idx, sym.depth() - 1});
            }
        }
        if (sym.type() == Symbol::Function && sym.ref() && sym.ref()->type() == Symbol::Function) {
            // unwrap function (self-)reference
            sym.set_index(sym.ref()->index());
        }
    }
    func.symtab().update_nonlocal_indices();
}


class NonlocalResolverVisitor final: public ast::VisitorExclTypes {
public:
    using VisitorExclTypes::visit;

    explicit NonlocalResolverVisitor(Function& func)
            : m_function(func) {}

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
        auto& my_symtab = m_function.symtab();
        const auto& symtab = *v.identifier.symbol.symtab();
        const auto& symptr = v.identifier.symbol;
        switch (symptr->type()) {
            case Symbol::Parameter: {
                // add Nonlocal symbol
                auto depth = my_symtab.depth(&symtab);
                if (depth > 0) {
                    auto found = my_symtab.find_last_of(v.identifier.name, Symbol::Nonlocal);
                    const TypeInfo& ti = symtab.function()->parameter(symptr->index());
                    if (found) {
                        assert(found->ref() == symptr);
                        v.identifier.symbol = found;
                        m_function.set_nonlocal(found->index(), TypeInfo{ti});
                    } else {
                        Index idx = my_symtab.count(Symbol::Nonlocal);
                        v.identifier.symbol = my_symtab.add({symptr, Symbol::Nonlocal, idx, depth});
                        v.identifier.symbol->set_callable(ti.is_callable());
                        m_function.set_nonlocal(idx, TypeInfo{ti});
                    }
                }
                break;
            }
            case Symbol::Function: {
                if (v.index == no_index) {
                    v.module = symtab.module() == nullptr ? &module() : symtab.module();
                    v.index = symptr->index();
                }
                auto& ref_fn = v.module->get_function(v.index);
                if (ref_fn.is_generic()) {
                    resolve_nonlocals(ref_fn, ref_fn.ast());
                } else {
                    resolve_nonlocals_in_symtab(ref_fn);
                }
                if (v.module != &module()) {
                    // Referenced function is from another module -> we're done
                    break;
                }
                // partial calls
                if (!m_function.partial().empty()) {
                    m_function.symtab().set_name(v.identifier.name + "/partial");
                    if (ref_fn.has_nonlocals()) {
                        auto nlsym = m_function.symtab().add({
                                v.identifier.symbol,
                                Symbol::Nonlocal,
                                Index(m_function.nonlocals().size()),
                                0});
                        m_function.add_nonlocal(TypeInfo{ref_fn.signature_ptr()});
                        v.identifier.symbol = nlsym;
                        //v.identifier.symbol->set_callable(true);
                    }
                    break;
                }
                // add Nonlocal symbol
                if (ref_fn.has_nonlocals()) {
                    auto depth = my_symtab.depth(&symtab);
                    if (depth > 0) {
                        auto found = my_symtab.find_last_of(v.identifier.name, Symbol::Nonlocal);
                        if (found) {
                            assert(found->ref() == symptr);
                            v.identifier.symbol = found;
                            m_function.set_nonlocal(found->index(), TypeInfo{ref_fn.signature_ptr()});
                        } else {
                            Index idx = my_symtab.count(Symbol::Nonlocal);
                            v.identifier.symbol = my_symtab.add({symptr, Symbol::Nonlocal, idx, depth});
                            v.identifier.symbol->set_callable(true);
                            m_function.set_nonlocal(idx, TypeInfo{ref_fn.signature_ptr()});
                        }
                    }
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
            auto& fn = module().get_function(v.partial_index);
            process_subroutine(fn, *v.callable);
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
        Function& fn = module().get_function(v.index);
        if (!fn.detect_generic()) {
            resolve_nonlocals(fn, v.body);
        }
    }

private:
    Module& module() { return m_function.module(); }

    void process_subroutine(Function& func, ast::Expression& expression) {
        NonlocalResolverVisitor visitor(func);
        expression.apply(visitor);
    }

private:
    Function& m_function;
};


void resolve_nonlocals(Function& func, const ast::Block& block)
{
    NonlocalResolverVisitor visitor {func};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
    resolve_nonlocals_in_symtab(func);
}


} // namespace xci::script
