// fold_dot_call.cpp created on 2020-01-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "fold_dot_call.h"
#include <xci/script/Function.h>
#include <xci/script/Module.h>

namespace xci::script {

using std::unique_ptr;


class FoldDotCallVisitor final: public ast::VisitorExclTypes {
public:
    using VisitorExclTypes::visit;

    explicit FoldDotCallVisitor(Function& func)
        : m_function(func) {}

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
        assert(!v.right_tmp);

        if (v.op.is_call()) {
            // First arg is the callable, second arg is a Call (with args)
            // Collapse inner Call into outer OpCall and move callable to it.
            assert(!v.callable);
            assert(v.args.size() == 2);
            m_collapsed = std::move(v.args[1]);
            auto* call = dynamic_cast<ast::Call*>(m_collapsed.get());
            assert(call != nullptr);
            assert(!call->callable);
            call->callable = std::move(v.args[0]);
        }
        else if (v.op.is_dot_call()) {
            // Collapse inner Call into outer OpCall (with op=DotCall)
            assert(!v.callable);
            assert(v.args.size() == 2);
            m_collapsed = std::move(v.args[1]);
            auto* call = dynamic_cast<ast::Call*>(m_collapsed.get());
            assert(call != nullptr);
            assert(call->callable);
            call->args.insert(call->args.begin(), std::move(v.args[0]));
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
        v.expression->apply(*this);
    }

    void visit(ast::Literal&) override {}

    void visit(ast::Tuple& v) override {
        for (auto& expr : v.items)
            apply_and_fold(expr);
    }

    void visit(ast::List& v) override {
        for (auto& expr : v.items)
            apply_and_fold(expr);
    }

    void visit(ast::StructInit& v) override {
        for (const auto& item : v.items)
            item.second->apply(*this);
    }

    void visit(ast::Reference&) override {}

    void visit(ast::Cast& v) override {
        v.expression->apply(*this);
    }

    void visit(ast::Class&) override {}

    void visit(ast::Instance& v) override {
        for (auto& dfn : v.defs)
            dfn.apply(*this);
    }

private:
    Module& module() { return m_function.module(); }

private:
    void apply_and_fold(unique_ptr<ast::Expression>& expr) {
        expr->apply(*this);
        if (m_collapsed) {
            expr = std::move(m_collapsed);
        }
    }

private:
    Function& m_function;
    unique_ptr<ast::Expression> m_collapsed;
};


void fold_dot_call(Function& func, const ast::Block& block)
{
    FoldDotCallVisitor visitor {func};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
}


} // namespace xci::script
