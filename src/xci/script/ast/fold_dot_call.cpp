// fold_dot_call.cpp created on 2020-01-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "fold_dot_call.h"
#include <xci/script/Function.h>
#include <xci/script/Module.h>

namespace xci::script {

using namespace std;


class FoldDotCallVisitor: public ast::Visitor {
public:
    explicit FoldDotCallVisitor(Function& func)
        : m_function(func) {}

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

        if (v.op.is_dot_call()) {
            // collapse inner Call into outer OpCall (with op=DotCall)
            assert(!v.right_tmp);
            assert(v.args.size() == 2);
            m_collapsed = move(v.args[1]);
            auto* call = dynamic_cast<ast::Call*>(m_collapsed.get());
            assert(call != nullptr);
            call->args.insert(call->args.begin(), std::move(v.args[0]));
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

    void visit(ast::Integer& v) override {}
    void visit(ast::Float& v) override {}
    void visit(ast::String& v) override {}
    void visit(ast::Tuple& v) override {}
    void visit(ast::List& v) override {}
    void visit(ast::Reference& v) override {}

    void visit(ast::Class& v) override {}
    void visit(ast::Instance& v) override {}

    void visit(ast::TypeName& t) final {}
    void visit(ast::FunctionType& t) final {}
    void visit(ast::ListType& t) final {}

private:
    Module& module() { return m_function.module(); }

private:
    void apply_and_fold(std::unique_ptr<ast::Expression>& expr) {
        expr->apply(*this);
        if (m_collapsed) {
            expr = move(m_collapsed);
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
