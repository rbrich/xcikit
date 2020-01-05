// Optimizer.cpp created on 2019-06-13, part of XCI toolkit
// Copyright 2019 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Optimizer.h"
#include "Value.h"
#include "Builtin.h"
#include "Function.h"
#include "Error.h"

namespace xci::script {

using namespace std;


class OptimizationVisitor: public ast::Visitor {
public:
    explicit OptimizationVisitor(Optimizer& processor, Function& func)
        : m_processor(processor), m_function(func) {}

    void visit(ast::Definition& dfn) override {
        dfn.expression->apply(*this);
        convert_const_object_to_expression();
        if (m_collapsed)
            dfn.expression = move(m_collapsed);
    }

    void visit(ast::Invocation& inv) override {
        inv.expression->apply(*this);
        convert_const_object_to_expression();
        if (m_collapsed)
            inv.expression = move(m_collapsed);
    }

    void visit(ast::Return& ret) override {
        ret.expression->apply(*this);
        convert_const_object_to_expression();
        if (m_collapsed)
            ret.expression = move(m_collapsed);
    }

    void visit(ast::Integer& v) override { set_const_value(make_unique<value::Int32>(v.value)); }
    void visit(ast::Float& v) override { set_const_value(make_unique<value::Float32>(v.value)); }
    void visit(ast::String& v) override { set_const_value(make_unique<value::String>(v.value)); }

    void visit(ast::Tuple& v) override {
        // TODO: const tuple -> static value
    }

    void visit(ast::List& v) override {
        // TODO: const list -> static value
    }

    void visit(ast::Reference& v) override {
        assert(v.identifier.symbol);
        auto& symtab = *v.identifier.symbol.symtab();
        auto& sym = *v.identifier.symbol;
        switch (sym.type()) {
            case Symbol::Module:
                set_variable_value(TypeInfo{Type::Module});
                return;
            case Symbol::Nonlocal:
                // TODO free variable
                //m_value = {};
                assert(!"not implemented");
                return;
            case Symbol::Value: {
                auto& val = symtab.module()->get_value(sym.index());
                set_const_value(val.make_copy());
                return;
            }
            case Symbol::Parameter:
                //m_value = m_function.scope().parameters().get(sym.index());
                assert(!"not implemented");
                return;
            case Symbol::Unresolved:
                assert(!"Optimizer: unresolved symbol");
                return;
            case Symbol::Instruction:
            case Symbol::Class:
            case Symbol::Instance:
            case Symbol::Method:
            case Symbol::TypeName:
            case Symbol::TypeVar:
            case Symbol::Function:
                break;
        }
    }

    void visit(ast::Call& v) override {
        Values const_args;
        int n_nonconst_args = 0;
        for (auto& arg : v.args) {
            arg->apply(*this);

            if (m_collapsed)
                arg = move(m_collapsed);

            if (m_is_const && n_nonconst_args == 0) {
                const_args.add(std::move(m_value));
            } else {
                n_nonconst_args ++;
            }
        }

        v.callable->apply(*this);
/*
        assert(sym.depth() == 0);
        auto& fn = module().get_function(sym.index());
        set_variable_value(TypeInfo{fn.signature_ptr()});
*/
        /*
        // Evaluate const function
        if (f.signature().params.size() <= const_args.size()) {
            f.
        }

        // Partially evaluate the function
        if (!const_args.empty()) {
            auto fr = m_value.as_function().partial_call(const_args);
            auto fi = m_compiler.module().functions().add(move(fr));
            m_value = m_compiler.module().functions().get(fi);
        }
            */

    }

    void visit(ast::OpCall& v) override {
        assert(!v.right_tmp);
        Values args;
        bool is_nonconst = false;
        for (auto& arg : v.args) {
            arg->apply(*this);
            if (m_collapsed)
                arg = move(m_collapsed);
            if (m_is_const) {
                args.add(std::move(m_value));
            } else {
                is_nonconst = true;
            }
        }

        if (is_nonconst) {
            set_variable_value(TypeInfo{Type::Unknown});
            return;
        }

        if (args.size() != 2)
            throw UnexpectedArgumentCount(2, args.size());

        // FIXME
//        auto f = builtin::binary_op_function(v.op.op);
//        set_const_value(f(args[0], args[1]));
    }

    void visit(ast::Condition& v) override {
        v.cond->apply(*this);
        if (m_collapsed)
            v.cond = move(m_collapsed);
        if (!m_is_const) {
            // try to collapse both branches
            v.then_expr->apply(*this);
            if (m_collapsed)
                v.then_expr = move(m_collapsed);
            v.else_expr->apply(*this);
            if (m_collapsed)
                v.else_expr = move(m_collapsed);
            set_variable_value(TypeInfo{Type::Unknown});
            return;
        }

        if (!m_value->is_bool())
            throw ConditionNotBool();

        // collapse const if expr
        if (dynamic_cast<value::Bool&>(*m_value).value()) {
            m_collapsed = move(v.then_expr);
        } else {
            m_collapsed = move(v.else_expr);
        }
        m_collapsed->apply(*this);
        convert_const_object_to_expression();
    }

    void visit(ast::Function& v) override {
        // collapse block with single statement
        if (v.body.statements.size() == 1) {
            auto* ret = dynamic_cast<ast::Return*>(v.body.statements[0].get());
            assert(ret != nullptr);
            m_collapsed = move(ret->expression);
            return;
        }

        Function& func = m_function.module().get_function(v.index);
        m_processor.process_block(func, v.body);

        set_variable_value(TypeInfo{Type::Unknown});
    }

    void visit(ast::Class& v) override {}
    void visit(ast::Instance& v) override {}

    void visit(ast::TypeName& t) final {}
    void visit(ast::FunctionType& t) final {}
    void visit(ast::ListType& t) final {}

private:
    Module& module() { return m_function.module(); }

    void set_const_value(unique_ptr<Value>&& v) {
        m_value = move(v);
        m_value_type = m_value->type_info();
        m_is_const = true;
    }

    void set_variable_value(TypeInfo&& ti) {
        m_value_type = move(ti);
        m_is_const = false;
    }

    void convert_const_object_to_expression() {
        if (!m_is_const)
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
        m_value->apply(visitor);
    }

private:
    Optimizer& m_processor;
    Function& m_function;
    BuiltinModule m_builtin_module;
    unique_ptr<Value> m_value;
    TypeInfo m_value_type;
    bool m_is_const = false;
    unique_ptr<ast::Expression> m_collapsed;
};


void Optimizer::process_block(Function& func, const ast::Block& block)
{
    OptimizationVisitor visitor {*this, func};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
}


} // namespace xci::script
