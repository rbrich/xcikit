// SymbolResolver.cpp created on 2019-06-14, part of XCI toolkit
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

#include "SymbolResolver.h"
#include "Function.h"
#include "Module.h"
#include "Builtin.h"
#include <vector>

using namespace std;

namespace xci::script {


class SymbolResolverVisitor: public ast::Visitor {
public:
    explicit SymbolResolverVisitor(Function& func) : m_function(func) {}

    void visit(ast::Definition& dfn) override {
        // check for name collision
        if (m_function.symtab().find_by_name(dfn.variable.identifier.name))
            throw MultipleDeclarationError(dfn.variable.identifier.name);

        // add new symbol
        dfn.variable.identifier.symbol = m_function.symtab().add({dfn.variable.identifier.name, Symbol::Value, no_index});
        m_definition = &dfn;
        dfn.expression->apply(*this);
        m_definition = nullptr;
    }

    void visit(ast::Invocation& inv) override {
        inv.expression->apply(*this);
    }

    void visit(ast::Return& ret) override {
        ret.expression->apply(*this);
    }

    void visit(ast::Integer& v) override {}
    void visit(ast::Float& v) override {}
    void visit(ast::String& v) override {}

    void visit(ast::Tuple& v) override {
        for (auto& item : v.items) {
            item->apply(*this);
        }
    }

    void visit(ast::List& v) override {
        for (auto& item : v.items) {
            item->apply(*this);
        }
    }

    void visit(ast::Reference& v) override {
        auto& symptr = v.identifier.symbol;
        symptr = resolve_symbol(v.identifier.name);
        if (!symptr)
            throw UndefinedName(v.identifier.name, v.source_info);
    }

    void visit(ast::Call& v) override {
        v.callable->apply(*this);
        for (auto& arg : v.args) {
            arg->apply(*this);
        }
    }

    void visit(ast::OpCall& v) override {
        assert(!v.right_tmp);
        v.callable = make_unique<ast::Reference>(ast::Identifier{builtin::op_to_function_name(v.op.op)});
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        v.cond->apply(*this);
        v.then_expr->apply(*this);
        v.else_expr->apply(*this);
    }

    void visit(ast::Function& v) override {
        std::string name = "<lambda>";
        if (v.type.params.empty())
            name = "<block>";
        if (m_definition != nullptr) {
            name = m_definition->variable.identifier.name;
            m_definition->variable.identifier.symbol->set_callable(true);
        }
        SymbolTable& symtab = m_function.symtab().add_child(name);
        size_t par_idx = 0;
        for (auto& p : v.type.params) {
            p.identifier.symbol = symtab.add({p.identifier.name, Symbol::Parameter, par_idx++});
        }

        auto fn = make_unique<Function>(module(), symtab);
        m_postponed_blocks.push_back({*fn, v.body});
        v.index = module().add_function(move(fn));
        v.body.symtab = &symtab;
    }

    void visit(ast::TypeName& t) final {}
    void visit(ast::FunctionType& t) final {}
    void visit(ast::ListType& t) final {}

    struct PostponedBlock {
        Function& func;
        const ast::Block& block;
    };
    const std::vector<PostponedBlock>& postponed_blocks() const { return m_postponed_blocks; }

private:
    Module& module() { return m_function.module(); }

    SymbolPointer resolve_symbol(const string& name) {
        // lookup intrinsics in builtin module first
        // (this is just optiomization, the same lookup is repeated below)
        if (name.size() > 3 && name[0] == '_' && name[1] == '_') {
            auto symptr = module().get_imported_module(0).symtab().find_by_name(name);
            if (symptr)
                return symptr;
        }
        // (non)local values and parameters
        {
            // lookup in this and parent symtabs
            size_t depth = 0;
            for (auto p_symtab = &m_function.symtab(); p_symtab != nullptr; p_symtab = p_symtab->parent()) {
                if (p_symtab->name() == name && p_symtab->parent() != nullptr) {
                    // recursion - unwrap the function
                    auto symptr = p_symtab->parent()->find_by_name(name);
                    return m_function.symtab().add({symptr, Symbol::Function, depth + 1});
                }

                auto symptr = p_symtab->find_by_name(name);
                if (symptr) {
                    if (depth > 0) {
                        // add Nonlocal symbol
                        return m_function.symtab().add({symptr, Symbol::Nonlocal, depth});
                    } else {
                        return symptr;
                    }
                }
                depth ++;
            }
        }
        // this module
        {
            auto symptr = module().symtab().find_by_name(name);
            if (symptr)
                return symptr;
        }
        // imported modules
        for (size_t i = 0; i < module().num_imported_modules(); i++) {
            auto symptr = module().get_imported_module(i).symtab().find_by_name(name);
            if (symptr)
                return symptr;
        }
        // nowhere
        return {};
    }

private:
    std::vector<PostponedBlock> m_postponed_blocks;
    Function& m_function;
    ast::Definition* m_definition = nullptr;  // symbol being currently defined
};


void SymbolResolver::process_block(Function& func, const ast::Block& block)
{
    SymbolResolverVisitor visitor {func};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }

    // process postponed blocks
    for (const auto& blk : visitor.postponed_blocks()) {
        process_block(blk.func, blk.block);
    }
}


class NonlocalResolverVisitor: public ast::Visitor {
public:
    explicit NonlocalResolverVisitor(NonlocalResolver& processor, Function& func)
        : m_processor(processor), m_function(func) {}

    void visit(ast::Definition& dfn) override {
        m_definition = &dfn;
        dfn.expression->apply(*this);
        m_definition = nullptr;
    }

    void visit(ast::Invocation& inv) override {
        inv.expression->apply(*this);
    }

    void visit(ast::Return& ret) override {
        ret.expression->apply(*this);
    }

    void visit(ast::Integer& v) override {}
    void visit(ast::Float& v) override {}
    void visit(ast::String& v) override {}

    void visit(ast::Tuple& v) override {
        for (auto& item : v.items) {
            item->apply(*this);
        }
    }

    void visit(ast::List& v) override {
        for (auto& item : v.items) {
            item->apply(*this);
        }
    }

    void visit(ast::Reference& v) override {}

    void visit(ast::Call& v) override {
        v.callable->apply(*this);
        for (auto& arg : v.args) {
            arg->apply(*this);
        }
    }

    void visit(ast::OpCall& v) override {
        visit(*static_cast<ast::Call*>(&v));
    }

    void visit(ast::Condition& v) override {
        v.cond->apply(*this);
        v.then_expr->apply(*this);
        v.else_expr->apply(*this);
    }

    void visit(ast::Function& v) override {
        Function& func = m_function.module().get_function(v.index);
        m_processor.process_block(func, v.body);
        for (auto& sym : func.symtab()) {
            if (sym.type() == Symbol::Nonlocal) {
                if (sym.ref() && sym.ref()->type() == Symbol::Function) {
                    // unfrap reference to non-value function
                    sym = *sym.ref();
                } else if (sym.depth() > 1) {
                    // not direct parent -> add intermediate Nonlocal
                    m_function.symtab().add({sym.ref(), Symbol::Nonlocal, sym.depth() - 1});
                }
            }
            if (sym.type() == Symbol::Function && sym.ref() && sym.ref()->type() == Symbol::Function) {
                // unwrap function (self-)reference
                sym.set_index(sym.ref()->index());
            }
        }
        if (m_definition != nullptr && func.symtab().count_nonlocals() == 0) {
            auto& sym = m_definition->variable.identifier.symbol;
            sym->set_type(Symbol::Function);
            sym->set_index(v.index);
        }
    }

    void visit(ast::TypeName& t) final {}
    void visit(ast::FunctionType& t) final {}
    void visit(ast::ListType& t) final {}

private:
    Module& module() { return m_function.module(); }

private:
    NonlocalResolver& m_processor;
    Function& m_function;
    ast::Definition* m_definition = nullptr;  // symbol being currently defined
};


void NonlocalResolver::process_block(Function& func, const ast::Block& block)
{
    NonlocalResolverVisitor visitor {*this, func};
    for (const auto& stmt : block.statements) {
        stmt->apply(visitor);
    }
    func.symtab().update_nonlocal_indices();
}


} // namespace xci::script
