// NonlocalResolver.cpp created on 2020-01-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "NonlocalResolver.h"
#include "Function.h"
#include "Module.h"

namespace xci::script {

using namespace std;


class NonlocalResolverVisitor: public ast::Visitor {
public:
    explicit NonlocalResolverVisitor(NonlocalResolver& processor, Function& func)
            : m_processor(processor), m_function(func) {}

    void visit(ast::Definition& dfn) override {
        dfn.expression->apply(*this);
    }

    void visit(ast::Invocation& inv) override {
        inv.expression->apply(*this);
    }

    void visit(ast::Return& ret) override {
        ret.expression->apply(*this);
    }

    void visit(ast::Class& v) override {}
    void visit(ast::Instance& v) override {}

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
        assert(v.identifier.symbol);
        const auto& symtab = *v.identifier.symbol.symtab();
        const auto& sym = *v.identifier.symbol;
        switch (sym.type()) {
            case Symbol::Function: {
                auto* symmod = symtab.module() == nullptr ? &module() : symtab.module();
                auto& fn = symmod->get_function(sym.index());
                if (!fn.is_native() && fn.has_ast()) {
                    process_function(fn, *fn.ast());
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

            /*auto it = m_function.symtab().begin();
            for (const auto& nl : fn.nonlocals()) {
                while (it->type() != Symbol::Parameter) {
                    ++it;
                }
                fn.symtab().add({{m_function.symtab(), Index(it - m_function.symtab().begin())},
                                 Symbol::Nonlocal, 1});
            }*/
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

    void visit(ast::Function& v) override {
        Function& func = module().get_function(v.index);
        process_function(func, v.body);

        if (!func.has_nonlocals())
            return;

        if (v.definition) {
            // create wrapping function for the closure
            SymbolTable& wfn_symtab = m_function.symtab().add_child(func.symtab().name() + "/closure");
            auto wfn = make_unique<Function>(module(), wfn_symtab);
            wfn->set_kind(Function::Kind::Inline);
            wfn->set_signature(func.signature_ptr());
            auto wfn_index = module().add_function(move(wfn));
            v.definition->symbol()->set_index(wfn_index);
            v.definition->symbol()->set_type(Symbol::Fragment);
        }
    }

    void visit(ast::TypeName& t) final {}
    void visit(ast::FunctionType& t) final {}
    void visit(ast::ListType& t) final {}

private:
    Module& module() { return m_function.module(); }

    void process_subroutine(Function& func, ast::Expression& expression) {
        NonlocalResolverVisitor visitor(m_processor, func);
        expression.apply(visitor);
    }

    void process_function(Function& func, ast::Block& body) {
        m_processor.process_block(func, body);

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
    NonlocalResolver& m_processor;
    Function& m_function;
};


void NonlocalResolver::process_block(Function& func, const ast::Block& block)
{
    NonlocalResolverVisitor visitor {*this, func};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
    func.symtab().update_nonlocal_indices();
}


} // namespace xci::script
