// fold_tuple.cpp created on 2021-02-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "fold_tuple.h"
#include <xci/script/Function.h>
#include <xci/script/Module.h>

namespace xci::script {

using std::unique_ptr;


class FoldTupleVisitor final: public ast::VisitorExclTypes {
public:
    using VisitorExclTypes::visit;

    void visit(ast::Definition& dfn) override {
        if (dfn.expression)
            apply_and_fold(dfn.expression);
    }

    void visit(ast::Invocation& inv) override {
        apply_and_fold(inv.expression);
    }

    void visit(ast::Return& ret) override {
        apply_and_fold(ret.expression);
    }

    void visit(ast::Call& v) override {
        if (v.arg)
            apply_and_fold(v.arg);
        if (v.callable)
            apply_and_fold(v.callable);
    }

    void visit(ast::OpCall& v) override {
        if (v.arg)
            apply_and_fold(v.arg);
        if (v.right_arg)
            apply_and_fold(v.right_arg);
        assert(!v.callable);

        if (v.op.is_comma()) {
            // collapse comma operator to tuple items
            assert(!v.right_tmp);
            m_collapsed = std::make_unique<ast::Tuple>();
            m_collapsed->source_loc = v.source_loc;
            auto fold = [this](std::unique_ptr<ast::Expression>& expr) {
                auto* tuple = dynamic_cast<ast::Tuple*>(expr.get());
                if (tuple == nullptr) {
                    // subexpr is not a tuple
                    m_collapsed->items.push_back(std::move(expr));
                } else {
                    // it is a tuple, fold it
                    std::move(std::begin(tuple->items), std::end(tuple->items),
                            std::back_inserter(m_collapsed->items));
                }
            };
            fold(v.arg);
            fold(v.right_arg);
        }
    }

    void visit(ast::Condition& v) override {
        for (auto& item : v.if_then_expr) {
            apply_and_fold(item.first);
            apply_and_fold(item.second);
        }
        apply_and_fold(v.else_expr);
    }

    void visit(ast::WithContext& v) override {
        apply_and_fold(v.context);
        apply_and_fold(v.expression);
    }

    void visit(ast::Function& v) override {
        for (const auto& stmt : v.body.statements) {
            stmt->apply(*this);
        }
    }

    void visit(ast::Parenthesized& v) override {
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

    void visit(ast::StructInit& v) override {
        for (const auto& item : v.items)
            item.second->apply(*this);
    }

    void visit(ast::Literal&) override {}
    void visit(ast::Tuple&) override {}
    void visit(ast::Reference&) override {}

    void visit(ast::Cast& v) override {
        apply_and_fold(v.expression);
    }

    void visit(ast::Class&) override {}

    void visit(ast::Instance& v) override {
        for (auto& dfn : v.defs)
            dfn.apply(*this);
    }

private:
    void apply_and_fold(unique_ptr<ast::Expression>& expr) {
        expr->apply(*this);
        if (m_collapsed) {
            expr = std::move(m_collapsed);
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
