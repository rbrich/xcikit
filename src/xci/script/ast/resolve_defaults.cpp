// resolve_defaults.cpp created on 2026-11-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2026 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "resolve_defaults.h"
#include <xci/script/Error.h>
#include <xci/script/Module.h>

#include <range/v3/view/enumerate.hpp>

namespace xci::script {

using ranges::views::enumerate;


class ResolveDefaultsVisitor final: public ast::Visitor {
public:
    explicit ResolveDefaultsVisitor(Scope& scope) : m_scope(scope) {}

    void visit(ast::Definition& dfn) override {
        if (dfn.expression) {
            dfn.expression->apply(*this);
        }
    }

    void visit(ast::Invocation& inv) override {}
    void visit(ast::Return& ret) override {}
    void visit(ast::Class& v) override {}
    void visit(ast::Instance& v) override {}

    void visit(ast::TypeDef& v) override {
        const auto idx = v.type_name.symbol->index();
        m_type_info = module().get_type(idx).underlying();

        v.type->apply(*this);

        const auto new_idx = module().update_type(idx, TypeInfo{v.type_name.name, std::move(m_type_info)});
        v.type_name.symbol->set_index(new_idx);
    }

    void visit(ast::TypeAlias& v) override {}

    void visit(ast::Literal& v) override {
        m_value = v.value;
    }

    void visit(ast::Tuple& v) override {}
    void visit(ast::List& v) override {}

    void visit(ast::StructInit& v) override {
        // Update the TypeInfo to the new one with defaults for the Compiler
        if (v.ti.is_named()) {
            // FIXME: It may come from a different module - must save SymbolPtr instead
            const auto idx = module().find_named_type(v.ti.name());
            if (idx != no_index)
                v.ti = module().get_type(idx);
        }
    }

    void visit(ast::Reference& v) override {}
    void visit(ast::Call& v) override {}
    void visit(ast::OpCall& v) override {}
    void visit(ast::Condition& v) override {}
    void visit(ast::WithContext& v) override {}
    void visit(ast::Function& v) override {}
    void visit(ast::Cast& v) override {}
    void visit(ast::TypeName& t) override {}
    void visit(ast::FunctionType& t) override {}
    void visit(ast::ListType& t) override {}
    void visit(ast::TupleType& t) override {}

    void visit(ast::StructType& t) override {
        for (auto&& [i, st] : t.subtypes | enumerate) {
            if (st.default_value) {
                m_value = {};
                st.default_value->apply(*this);
                if (m_value.is_unknown())
                    throw struct_default_not_constant(st.identifier.name, st.default_value->source_loc);

                // Store the literal value as a static in the module
                const Index static_idx = module().add_value(std::move(m_value));

                // Update the TypeInfo stored in the module via m_type_info - see TypeDef
                auto& ti = m_type_info.subtypes()[i];
                ti.set_default_value(static_idx);
            }
        }
    }

private:
    Module& module() { return m_scope.module(); }

    Scope& m_scope;

    TypedValue m_value;
    TypeInfo m_type_info;
};


void resolve_defaults(Scope& scope, const ast::Block& block)
{
    ResolveDefaultsVisitor visitor {scope};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
}


} // namespace xci::script
