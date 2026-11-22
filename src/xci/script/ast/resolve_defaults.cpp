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
        if (dfn.variable.type) {
            Function& fn = dfn.symbol().get_function(m_scope);
            if (fn.has_nonvoid_parameter()) {
                m_type_info = TypeInfo(fn.signature_ptr());
                dfn.variable.type->apply(*this);
            } else {
                m_type_info = fn.signature().return_type;
                dfn.variable.type->apply(*this);
            }
        }
        if (dfn.expression) {
            dfn.expression->apply(*this);
        }
        m_type_info = {};
    }

    void visit(ast::Invocation& inv) override {}

    void visit(ast::Return& ret) override {
        ret.expression->apply(*this);
    }

    void visit(ast::Class& v) override {}
    void visit(ast::Instance& v) override {}

    void visit(ast::TypeDef& v) override {
        const auto idx = v.type_name.symbol->index();
        m_type_info = module().get_type(idx);

        v.type->apply(*this);

        const auto new_idx = module().update_type(idx, std::move(m_type_info));
        v.type_name.symbol->set_index(new_idx);
    }

    void visit(ast::TypeAlias& v) override {}

    void visit(ast::Literal& v) override {
        m_value = v.value;
    }

    void visit(ast::Tuple& v) override {
        if (v.ti.is_struct_or_tuple() && m_type_info.is_struct()) {
            v.ti = std::move(m_type_info);
            for (auto&& [i, item] : v.items | enumerate) {
                m_type_info = v.ti.subtypes()[i];
                item->apply(*this);
                m_type_info = {};
            }
        }
    }

    void visit(ast::List& v) override {}

    void visit(ast::StructInit& v) override {
        if (v.ti.is_struct() && m_type_info.is_struct()) {
            v.ti = std::move(m_type_info);
            for (auto&& [i, item] : v.items | enumerate) {
                m_type_info = v.ti.subtypes()[i];
                item.second->apply(*this);
                m_type_info = {};
            }
        }
    }

    void visit(ast::Reference& v) override {
        assert(v.identifier.symbol);
        const auto& sym = *v.identifier.symbol;
        switch (sym.type()) {
            case Symbol::Instruction:
            case Symbol::TypeIndex:
            case Symbol::Module:
            case Symbol::Nonlocal:
            case Symbol::Value:
            case Symbol::Parameter:
            case Symbol::Method:
                m_type_info = {};
                break;
            case Symbol::Function: {
                m_type_info = v.ti;
                if (v.module != &module()) {
                    // calling function from different module - copy defaults (if any)
                    auto& param = m_type_info.signature().param_type;
                    if (param.is_struct()) {
                        for (auto& item : param.subtypes()) {
                            auto idx = item.default_value();
                            const TypedValue& val = v.module->get_value(idx);
                            val.incref();
                            idx = module().add_value(TypedValue(val));
                            item.set_default_value(idx);
                        }
                    }
                }
                break;
            }
            case Symbol::Class:
                assert(!"Class cannot be called.");
                break;
            case Symbol::Instance:
                assert(!"Instance cannot be called.");
                break;
            case Symbol::TypeName:
            case Symbol::TypeAlias:
            case Symbol::TypeVar:
            case Symbol::StructItem:
                m_type_info = {};
                break;
            case Symbol::Unresolved:
                XCI_UNREACHABLE;
        }
    }

    void visit(ast::Call& v) override {
        v.callable->apply(*this);

        if (m_type_info.is_callable()) {
            const auto& param_type = m_type_info.ul_signature().param_type;
            if (v.arg && param_type.is_struct()) {
                // Propagate struct defaults from the function's parameter type to the arg
                m_type_info = param_type;
                v.arg->apply(*this);
                m_type_info = {};
            }
        }
    }

    void visit(ast::OpCall& v) override {
        visit(*static_cast<ast::Call*>(&v));
    }
    void visit(ast::Condition& v) override {}
    void visit(ast::WithContext& v) override {}

    void visit(ast::Function& v) override {
        Scope& scope = module().get_scope(v.scope_index);
        const Function& fn = scope.function();
        if (fn.has_nonvoid_parameter()) {
            m_type_info = TypeInfo(fn.signature_ptr());
            v.type.apply(*this);
            m_type_info = {};
        }

        // resolve body
        resolve_defaults(scope, v.body);
    }

    void visit(ast::Cast& v) override {}
    void visit(ast::TypeName& t) override {}

    void visit(ast::FunctionType& t) override {
        if (t.param.type) {
            TypeInfo fn_ti = std::move(m_type_info);
            Signature& sig = fn_ti.signature();
            m_type_info = std::move(sig.param_type);
            t.param.type->apply(*this);
            sig.param_type = std::move(m_type_info);
            m_type_info = std::move(fn_ti);
        }
    }

    void visit(ast::ListType& t) override {}
    void visit(ast::TupleType& t) override {}

    void visit(ast::StructType& t) override {
        for (auto&& [i, st] : t.subtypes | enumerate) {
            // Evaluate the item type, in case it is also a StructType
            if (st.type) {
                auto struct_ti = m_type_info;
                m_type_info = struct_ti.subtypes()[i];
                st.type->apply(*this);
                struct_ti.subtypes()[i] = std::move(m_type_info);
                m_type_info = std::move(struct_ti);
            }

            // Evaluate default value, update it in m_type_info.subtypes()
            if (st.default_value) {
                m_value = {};
                st.default_value->apply(*this);
                if (m_value.is_unknown())
                    throw struct_default_not_constant(st.identifier.name, st.default_value->source_loc);

                // Store the literal value as a static in the module
                m_value.incref();
                const Index static_idx = module().add_value(std::move(m_value));

                // Update the TypeInfo stored in the module via m_type_info - see TypeDef, Definition, FunctionType
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
