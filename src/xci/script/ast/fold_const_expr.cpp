// fold_const_expr.cpp created on 2019-06-13 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "fold_const_expr.h"
#include <xci/script/Value.h>
#include <xci/script/Module.h>
#include <xci/script/Function.h>
#include <xci/script/Machine.h>
#include <range/v3/view/reverse.hpp>

namespace xci::script {

using std::unique_ptr;
using std::make_unique;
using std::move;


class OptimizationVisitor final: public ast::Visitor {
public:
    explicit OptimizationVisitor(Function& func)
        : m_function(func) {}

    void visit(ast::Definition& dfn) override {
        m_const_value.reset();
        apply_and_fold(dfn.expression);
    }

    void visit(ast::Invocation& inv) override {
        m_const_value.reset();
        apply_and_fold(inv.expression);
    }

    void visit(ast::Return& ret) override {
        m_const_value.reset();
        apply_and_fold(ret.expression);
    }

    void visit(ast::Reference& v) override {
        assert(v.identifier.symbol);
        auto& symtab = *v.identifier.symbol.symtab();
        auto& sym = *v.identifier.symbol;
        switch (sym.type()) {
            case Symbol::Module:
                break;
            case Symbol::Nonlocal:
                break;
            case Symbol::Value: {
                m_const_value = symtab.module()->get_value(sym.index()).make_copy();
                return;
            }
            case Symbol::Parameter:
                //m_value = m_function.scope().parameters().get(sym.index());
                break;
            case Symbol::Instruction:
            case Symbol::Class:
            case Symbol::Instance:
            case Symbol::Method:
            case Symbol::TypeName:
            case Symbol::TypeVar:
                break;
            case Symbol::Function: {
                auto& symmod = symtab.module() == nullptr ? module() : *symtab.module();
                Function& fn = symmod.get_function(sym.index());
                if (fn.is_compiled()) {
                    m_const_value = make_unique<value::Closure>(fn);
                    return;
                }
                break;
            }
            case Symbol::Fragment:
                break;
            case Symbol::Unresolved:
                assert(!"Optimizer: unresolved symbol");
                break;
        }
        m_const_value.reset();
    }

    void visit(ast::Call& v) override {
        Values args;
        bool all_const = true;
        for (auto& arg : v.args) {
            apply_and_fold(arg);
            if (all_const && m_const_value) {
                args.add(move(m_const_value));
            } else {
                all_const = false;
            }
        }
        apply_and_fold(v.callable);

        if (all_const && m_const_value) {
            // prepare to run the function in compile-time
            // + sanity checks
            assert(m_const_value->type() == Type::Function);
            auto& fnval = m_const_value->as<value::Closure>();
            assert(!*fnval.heapslot());  // no values in closure
            auto& fn = fnval.function();
            assert(!fn.has_nonlocals());
            assert(fn.parameters().size() == args.size());
            // push args on stack
            for (auto& arg : ranges::views::reverse(args))
                m_machine.stack().push(*arg);
            // run it
            bool invoked = false;
            m_machine.call(fn, [&invoked](const Value&){ invoked = true; });
            if (!invoked) {
                assert(m_machine.stack().size() == fn.effective_return_type().size());
                m_const_value = m_machine.stack().pull(fn.effective_return_type());
            } else {
                // backoff - can't process invocations in compile-time
                m_const_value.reset();
            }
            return;
        }
        m_const_value.reset();
    }

    void visit(ast::OpCall& v) override {
        assert(!v.right_tmp);
        assert(v.callable);
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        m_const_value.reset();
        apply_and_fold(v.cond);

        if (!m_const_value) {
            // try to collapse each branch
            apply_and_fold(v.then_expr);
            apply_and_fold(v.else_expr);
            m_const_value.reset();
            return;
        }

        // collapse const if-expr
        assert(m_const_value->is_bool());
        if (m_const_value->as<value::Bool>().value()) {
            apply_and_fold(v.then_expr);
            m_collapsed = move(v.then_expr);
        } else {
            apply_and_fold(v.else_expr);
            m_collapsed = move(v.else_expr);
        }
    }

    void visit(ast::Function& v) override {
        // collapse block with single statement
        if (v.body.statements.size() == 1) {
            auto* ret = dynamic_cast<ast::Return*>(v.body.statements[0].get());
            assert(ret != nullptr);
            apply_and_fold(ret->expression);
            m_collapsed = move(ret->expression);
            return;
        }

        Function& func = m_function.module().get_function(v.index);
        fold_const_expr(func, v.body);
        m_const_value.reset();
    }

    void visit(ast::Integer& v) override { m_const_value = make_unique<value::Int32>(v.value); }
    void visit(ast::Float& v) override { m_const_value = make_unique<value::Float32>(v.value); }
    void visit(ast::String& v) override { m_const_value = make_unique<value::String>(v.value); }

    void visit(ast::Tuple& v) override {
        // TODO: const tuple -> static value
        m_const_value.reset();
    }

    void visit(ast::List& v) override {
        // TODO: const list -> static value
        m_const_value.reset();
    }

    void visit(ast::Class& v) override { m_const_value.reset(); }
    void visit(ast::Instance& v) override { m_const_value.reset(); }

    void visit(ast::TypeName& t) final {}
    void visit(ast::FunctionType& t) final {}
    void visit(ast::ListType& t) final {}

private:
    Module& module() { return m_function.module(); }

    void apply_and_fold(unique_ptr<ast::Expression>& expr) {
        expr->apply(*this);
        convert_const_object_to_expression();
        if (m_collapsed) {
            expr = move(m_collapsed);
        }
    }

    void convert_const_object_to_expression() {
        if (!m_const_value)
            return;
        struct ValueVisitor: public value::Visitor {
            unique_ptr<ast::Expression>& collapsed;
            explicit ValueVisitor(unique_ptr<ast::Expression>& collapsed) : collapsed(collapsed) {}
            void visit(const value::Void&) override {}
            void visit(const value::Bool&) override {}
            void visit(const value::Byte&) override {}
            void visit(const value::Char&) override {}
            void visit(const value::Int32& v) override { collapsed = make_unique<ast::Integer>(v.value()); }
            void visit(const value::Int64& v) override {}
            void visit(const value::Float32& v) override { collapsed = make_unique<ast::Float>(v.value()); }
            void visit(const value::Float64&) override {}
            void visit(const value::String& v) override { collapsed = make_unique<ast::String>(v.value()); }
            void visit(const value::List& v) override {}
            void visit(const value::Tuple& v) override {}
            void visit(const value::Closure&) override {}
            void visit(const value::Module& v) override {}
        };
        ValueVisitor visitor(m_collapsed);
        m_const_value->apply(visitor);
    }

private:
    Function& m_function;
    Machine m_machine;  // VM for evaluation of constant functions
    unique_ptr<Value> m_const_value;
    unique_ptr<ast::Expression> m_collapsed;
};


void fold_const_expr(Function& func, const ast::Block& block)
{
    OptimizationVisitor visitor {func};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
}


} // namespace xci::script
