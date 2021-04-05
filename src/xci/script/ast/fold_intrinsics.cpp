// fold_intrinsics.cpp created on 2021-04-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "fold_intrinsics.h"
#include <xci/script/Error.h>

namespace xci::script {


class FoldIntrinsicsVisitor final: public ast::Visitor {
public:
    void visit(ast::Definition& v) override { v.expression->apply(*this); reset(); }
    void visit(ast::Invocation& v) override { v.expression->apply(*this); reset(); }
    void visit(ast::Return& v) override { v.expression->apply(*this); reset(); }

    void visit(ast::Reference& v) override {
        assert(v.identifier.symbol);
        if (v.identifier.symbol->type() == Symbol::Instruction)
            m_instr_ref = &v;
    }

    void visit(ast::Literal& v) override {
        if (!m_instr_ref)
            return;
        struct ValueVisitor: public value::PartialVisitor {
            uint8_t & val;
            const SourceInfo& si;
            explicit ValueVisitor(uint8_t& val, const SourceInfo& si) : val(val), si(si) {}
            void visit(const value::Byte& v) override { val = v.value(); }
            void visit(const value::Int32& v) override {
                auto i = v.value();
                if (i < 0 || i > 255)
                    throw IntrinsicsFunctionError("arg value out of Byte range: " + std::to_string(i), si);
                val = i;
            }
        };
        ValueVisitor visitor(m_arg_value, v.source_info);
        v.value->apply(visitor);
    }

    void visit(ast::Call& v) override {
        v.callable->apply(*this);
        unsigned arg_i = 0;
        for (auto& arg : v.args) {
            arg->apply(*this);
            if (m_instr_ref)
                m_instr_ref->instruction_args[arg_i] = m_arg_value;
        }
        if (m_instr_ref) {
            v.args.clear();
            reset();
        }
    }

    void visit(ast::OpCall& v) override {
        for (auto& arg : v.args)
            arg->apply(*this);
        reset();
    }

    void visit(ast::Condition& v) override {
        v.cond->apply(*this);
        v.then_expr->apply(*this);
        v.else_expr->apply(*this);
        reset();
    }

    void visit(ast::Function& v) override {
        for (const auto& stmt : v.body.statements) {
            stmt->apply(*this);
        }
    }

    void visit(ast::Bracketed& v) override {
        v.expression->apply(*this);
    }

    void visit(ast::List& v) override {}
    void visit(ast::Tuple& v) override {}

    void visit(ast::Cast& v) override {
        v.expression->apply(*this);
    }

    void visit(ast::Class&) override {}
    void visit(ast::Instance& v) override {
        for (auto& d : v.defs)
            visit(d);
    }

    void visit(ast::TypeName&) final {}
    void visit(ast::FunctionType&) final {}
    void visit(ast::ListType&) final {}

private:
    void reset() {
        m_instr_ref = nullptr; }

    uint8_t m_arg_value = 0;
    ast::Reference* m_instr_ref = nullptr;  // set if inside a Reference to an Instruction
};


void fold_intrinsics(const ast::Block& block)
{
    FoldIntrinsicsVisitor visitor;
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
}


} // namespace xci::script