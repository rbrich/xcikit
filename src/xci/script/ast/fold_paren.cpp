// fold_paren.cpp created on 2023-05-25 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "fold_paren.h"
#include <xci/script/Function.h>
#include <xci/script/Module.h>

namespace xci::script {


class FoldParenVisitor final: public ast::VisitorExclTypes {
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
        for (auto& arg : v.args) {
            apply_and_fold(arg);
        }
        if (v.callable)
            apply_and_fold(v.callable);
    }

    void visit(ast::OpCall& v) override {
        for (auto& arg : v.args) {
            apply_and_fold(arg);
        }
        if (v.callable)
            apply_and_fold(v.callable);
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
        m_collapsed = std::move(v.expression);
    }

    void visit(ast::Tuple& v) override {
        for (auto& expr : v.items)
            apply_and_fold(expr);
    }

    void visit(ast::List& v) override {
        for (auto& expr : v.items)
            apply_and_fold(expr);
    }

    void visit(ast::StructInit& v) override {
        for (auto& item : v.items)
            apply_and_fold(item.second);
    }

    void visit(ast::Literal&) override {}
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
    void apply_and_fold(std::unique_ptr<ast::Expression>& expr) {
        expr->apply(*this);
        if (m_collapsed) {
            expr = std::move(m_collapsed);
        }
    }

private:
    std::unique_ptr<ast::Expression> m_collapsed;
};


void fold_paren(const ast::Block& block)
{
    FoldParenVisitor visitor;
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
}


} // namespace xci::script
