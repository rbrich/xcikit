// fold_const_expr.cpp created on 2019-06-13 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "fold_const_expr.h"
#include <xci/script/Value.h>
#include <xci/script/Module.h>
#include <xci/script/Function.h>
#include <xci/script/Machine.h>
#include <range/v3/view/reverse.hpp>
#include <optional>

namespace xci::script {

using ranges::cpp20::views::reverse;
using std::unique_ptr;
using std::make_unique;
using std::optional;


class FoldConstExprVisitor final: public ast::VisitorExclTypes {
public:
    using VisitorExclTypes::visit;

    explicit FoldConstExprVisitor(Function& func)
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
                m_const_value = symtab.module()->get_value(sym.index());
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
            case Symbol::StructItem:
            case Symbol::TypeIndex:
                break;
            case Symbol::Function: {
                assert(v.index != no_index);
                Function& fn = v.module->get_scope(v.index).function();
                if (fn.has_code()) {
                    m_const_value = TypedValue(value::Closure(fn), TypeInfo{fn.signature_ptr()});
                    return;
                }
                break;
            }
            case Symbol::Unresolved:
                assert(!"Optimizer: unresolved symbol");
                break;
        }
        m_const_value.reset();
    }

    void visit(ast::Call& v) override {
        TypedValue arg;
        bool const_arg = false;
        if (v.arg) {
            apply_and_fold(v.arg);
            if (m_const_value) {
                const_arg = true;
                arg = std::move(*m_const_value);
                m_const_value.reset();
            }
        }

        apply_and_fold(v.callable);
        if (const_arg && m_const_value) {
            // prepare to run the function in compile-time
            // + sanity checks
            assert(m_const_value->type() == Type::Function);
            auto& fnval = m_const_value->value().get<ClosureV>();
            assert(fnval.closure().empty());  // no values in closure
            auto& fn = *fnval.function();
            assert(!fn.has_nonlocals());
            assert(fn.parameters().size() == 1 ||
                   (arg.type_info().is_tuple() && arg.type_info().subtypes().size() == fn.parameters().size()));
            // push arg on stack
            m_machine.stack().push(arg);
            // run it
            bool invoked = false;
            m_machine.call(fn, [&invoked](const TypedValue&){ invoked = true; });
            if (!invoked) {
                auto reti = fn.effective_return_type();
                assert(m_machine.stack().size() == reti.size());
                m_const_value->decref();  // fnval
                m_const_value = m_machine.stack().pull_typed(reti);
                m_collapsed = make_unique<ast::Literal>(*m_const_value);
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
        bool all_const = true;  // all previous branches had const condition
        std::unique_ptr<ast::Expression> replacement_else_expr;
        for (auto& item : v.if_then_expr) {
            // condition
            apply_and_fold(item.first);
            // collapse when the condition is const expression
            if (m_const_value) {
                assert(m_const_value->is_bool());
                if (m_const_value->get<bool>()) {
                    apply_and_fold(item.second);
                    if (all_const) {
                        m_collapsed = std::move(item.second);
                        return;
                    } else {
                        // remove this branch - mark it, sweep below
                        // (The condition is const-true, so we can collapse
                        // the then-expression into the following else-expression.)
                        item.first.reset();
                        if (!replacement_else_expr)
                            replacement_else_expr = std::move(item.second);
                    }
                } else {
                    // remove this branch - mark it, sweep below
                    item.first.reset();
                    item.second.reset();
                }
            } else {
                if (replacement_else_expr) {
                    // remove this branch - mark it, sweep below
                    // (We've already encountered const-true condition.)
                    item.first.reset();
                    item.second.reset();
                } else {
                    all_const = false;
                    apply_and_fold(item.second);
                }
            }
            m_const_value.reset();
        }
        // sweep deleted branches
        std::erase_if(v.if_then_expr, [](const auto& it){ return !it.first; });

        if (v.if_then_expr.empty()) {
            // all branches removed, collapse the whole if to only the else-expression
            apply_and_fold(v.else_expr);
            m_collapsed = std::move(v.else_expr);
        } else {
            // try to collapse the else branch
            if (replacement_else_expr) {
                v.else_expr = std::move(replacement_else_expr);
            }
            apply_and_fold(v.else_expr);
            m_const_value.reset();
        }
    }

    void visit(ast::WithContext& v) override {
        m_const_value.reset();
        apply_and_fold(v.context);
        apply_and_fold(v.expression);
    }

    void visit(ast::Function& v) override {
        Function& func = module().get_scope(v.scope_index).function();

        // collapse block with single statement
        if (!func.has_nonvoid_parameters() && v.body.statements.size() == 1) {
            auto* ret = dynamic_cast<ast::Return*>(v.body.statements[0].get());
            assert(ret != nullptr);
            apply_and_fold(ret->expression);
            m_collapsed = std::move(ret->expression);
            return;
        }

        fold_const_expr(func, v.body);
        m_const_value.reset();
    }

    void visit(ast::Literal& v) override {
        m_const_value = v.value;
    }

    void visit(ast::Tuple& v) override {
        // TODO: const tuple -> static value
        m_const_value.reset();
    }

    void visit(ast::List& v) override {
        // TODO: const list -> static value
        m_const_value.reset();
    }

    void visit(ast::StructInit& v) override {
        m_const_value.reset();
    }

    void visit(ast::Cast& v) override {
        apply_and_fold(v.expression);
        // cast to Void?
        if (v.to_type.is_void()) {
            m_const_value = TypedValue(ti_void());
            m_collapsed = make_unique<ast::Literal>(*m_const_value);
            return;
        }
        if (!m_const_value)
            return;
        // cast to the same type?
        if (m_const_value->type_info() == v.to_type) {
            m_collapsed = make_unique<ast::Literal>(*m_const_value);
            return;
        }
        // FIXME: evaluate the actual (possibly user-defined) cast function
        auto cast_result = create_value(v.to_type);
        if (cast_result.cast_from(m_const_value->value())) {
            // fold the cast into value
            m_const_value = TypedValue(std::move(cast_result), v.to_type);
            m_collapsed = make_unique<ast::Literal>(*m_const_value);
            return;
        }
        m_const_value.reset();
    }

    void visit(ast::Class& v) override { m_const_value.reset(); }
    void visit(ast::Instance& v) override { m_const_value.reset(); }

private:
    Module& module() { return m_function.module(); }

    void apply_and_fold(unique_ptr<ast::Expression>& expr) {
        expr->apply(*this);  // may set either m_const_value or m_collapsed
        if (m_collapsed) {
            auto source_loc = expr->source_loc;
            expr = std::move(m_collapsed);
            if (!expr->source_loc)
                expr->source_loc = source_loc;
        }
    }

    Function& m_function;
    Machine m_machine;  // VM for evaluation of constant functions
    optional<TypedValue> m_const_value;
    unique_ptr<ast::Expression> m_collapsed;
};


void fold_const_expr(Function& func, const ast::Block& block)
{
    FoldConstExprVisitor visitor {func};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
}


} // namespace xci::script
