// resolve_nonlocals.cpp created on 2020-01-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "resolve_nonlocals.h"
#include <xci/script/Function.h>
#include <xci/script/Module.h>

namespace xci::script {


class NonlocalResolverVisitor final: public ast::VisitorExclTypes {
public:
    using VisitorExclTypes::visit;

    explicit NonlocalResolverVisitor(Function& func)
            : m_function(func) {}

    void visit(ast::Definition& dfn) override {
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

    void visit(ast::Bracketed& v) override {
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
        const auto& sym = *v.identifier.symbol;
        switch (sym.type()) {
            case Symbol::Nonlocal: {
                auto& nl_sym = *sym.ref();
                auto* nl_func = sym.ref().symtab()->function();
                assert(nl_func != nullptr);
                switch (nl_sym.type()) {
                    case Symbol::Function: {
                        auto& ref_fn = nl_func->module().get_function(nl_sym.index());
                        if (!ref_fn.has_nonlocals()) {
                            // eliminate nonlocal function without closure
                            v.identifier.symbol = sym.ref();
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case Symbol::Function: {
                auto* symmod = symtab.module() == nullptr ? &module() : symtab.module();
                auto& fn = symmod->get_function(sym.index());
                if (fn.is_generic()) {
                    assert(!fn.is_ast_copied());   // AST is referenced
                    process_function(fn, fn.ast());
                }
                break;
            }
            case Symbol::Fragment: {
                // only relevant for partial calls
                if (m_function.partial().empty())
                    break;
                assert(symtab.module() == nullptr || symtab.module() == &module());
                Function& fn = module().get_function(sym.index());
                m_function.symtab().set_name(v.identifier.name + "/partial");
                auto nlsym = m_function.symtab().add({v.identifier.symbol, Symbol::Nonlocal, 0});
                nlsym->set_index(m_function.nonlocals().size());
                m_function.add_nonlocal(TypeInfo{fn.signature_ptr()});
                v.identifier.symbol = nlsym;
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
        v.cond->apply(*this);
        v.then_expr->apply(*this);
        v.else_expr->apply(*this);
    }

    void visit(ast::WithContext& v) override {
        v.context->apply(*this);
        v.expression->apply(*this);
    }

    void visit(ast::Function& v) override {
        Function& func = module().get_function(v.index);
        process_function(func, v.body);

        if (!func.has_nonlocals())
            return;

        if (v.definition) {
            // create wrapping function for the closure
            SymbolTable& wfn_symtab = m_function.symtab().add_child(func.symtab().name() + "/closure");
            auto wfn = std::make_unique<Function>(module(), wfn_symtab);
            wfn->set_fragment();
            wfn->set_signature(func.signature_ptr());
            auto wfn_index = module().add_function(move(wfn));
            v.definition->symbol()->set_index(wfn_index);
            v.definition->symbol()->set_type(Symbol::Fragment);
        }
    }

private:
    Module& module() { return m_function.module(); }

    void process_subroutine(Function& func, ast::Expression& expression) {
        NonlocalResolverVisitor visitor(func);
        expression.apply(visitor);
    }

    void process_function(Function& func, const ast::Block& body) {
        resolve_nonlocals(func, body);

        auto& nonlocals = func.signature().nonlocals;
        if (nonlocals.empty())
            return;

        auto nonlocals_erased = 0;
        for (auto& sym : func.symtab()) {
            if (sym.type() == Symbol::Nonlocal) {
                sym.set_index(sym.index() - nonlocals_erased);
                if (sym.ref() && sym.ref()->type() == Symbol::Function) {
                    // unwrap reference to non-value function
                    nonlocals.erase(nonlocals.begin() + sym.index());
                    ++ nonlocals_erased;
                    sym = *sym.ref();
                } else if (sym.depth() > 1) {
                    // not direct parent -> add intermediate Nonlocal
                    auto ti = sym.ref().symtab()->function()->parameter(sym.ref()->index());
                    m_function.add_nonlocal(std::move(ti));
                    m_function.symtab().add({sym.ref(), Symbol::Nonlocal, sym.depth() - 1});
                }
            }
            if (sym.type() == Symbol::Function && sym.ref() && sym.ref()->type() == Symbol::Function) {
                // unwrap function (self-)reference
                sym.set_index(sym.ref()->index());
            }
        }
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
    func.symtab().update_nonlocal_indices();
}


} // namespace xci::script
