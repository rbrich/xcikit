// fold_tuple.cpp created on 2021-02-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "fold_tuple.h"
#include <xci/script/Function.h>
#include <xci/script/Module.h>

namespace xci::script {

using std::unique_ptr;
using std::move;


class FoldTupleVisitor final: public ast::Visitor {
public:
    void visit(ast::Definition& dfn) override {
        apply_and_fold(dfn.expression);
    }

    void visit(ast::Invocation& inv) override {
        apply_and_fold(inv.expression);
    }

    void visit(ast::Return& ret) override {
        apply_and_fold(ret.expression);
    }

    void visit(ast::Call& v) override {
        for (auto& arg : v.args) {
            apply_and_fold(arg);
        }
        apply_and_fold(v.callable);
    }

    void visit(ast::OpCall& v) override {
        for (auto& arg : v.args) {
            apply_and_fold(arg);
        }
        assert(!v.callable);

        if (v.op.is_comma()) {
            // collapse comma operator to tuple items
            assert(!v.right_tmp);
            m_collapsed = std::make_unique<ast::Tuple>();
            for (auto& expr : v.args) {
                auto* tuple = dynamic_cast<ast::Tuple*>(expr.get());
                if (tuple == nullptr) {
                    // subexpr is not a tuple
                    m_collapsed->items.push_back(std::move(expr));
                } else {
                    // it is a tuple, fold it
                    std::move(std::begin(tuple->items), std::end(tuple->items),
                            std::back_inserter(m_collapsed->items));
                }
            }
        }
    }

    void visit(ast::Condition& v) override {
        apply_and_fold(v.cond);
        apply_and_fold(v.then_expr);
        apply_and_fold(v.else_expr);
    }

    void visit(ast::Function& v) override {
        for (const auto& stmt : v.body.statements) {
            stmt->apply(*this);
        }
    }

    void visit(ast::Bracketed& v) override {
        apply_and_fold(v.expression);
    }

    void visit(ast::List& v) override {
        assert(v.items.size() <= 1);
        for (auto& item : v.items) {
            item->apply(*this);
            if (m_collapsed) {
                v.items = std::move(m_collapsed->items);
                m_collapsed.reset();
            }
        }
    }

    void visit(ast::Literal&) override {}
    void visit(ast::Tuple&) override {}
    void visit(ast::Reference&) override {}

    void visit(ast::Cast& v) override {
        apply_and_fold(v.expression);
    }

    void visit(ast::Class&) override {}
    void visit(ast::Instance&) override {}
    void visit(ast::TypeDef&) override {}

    void visit(ast::TypeName&) final {}
    void visit(ast::FunctionType&) final {}
    void visit(ast::ListType&) final {}

private:
    void apply_and_fold(unique_ptr<ast::Expression>& expr) {
        expr->apply(*this);
        if (m_collapsed) {
            expr = move(m_collapsed);
        }
    }

private:
    unique_ptr<ast::Tuple> m_collapsed;
};


void fold_tuple(const ast::Block& block)
{
    FoldTupleVisitor visitor;
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
}


} // namespace xci::script
