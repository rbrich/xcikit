// SymbolTable.cpp created on 2019-07-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "SymbolTable.h"
#include "Module.h"
#include <cassert>
#include <algorithm>

namespace xci::script {


const Symbol& SymbolPointer::operator*() const
{
    assert(m_symtab != nullptr);
    return m_symtab->get(m_symidx);
}


const Symbol* SymbolPointer::operator->() const
{
    assert(m_symtab != nullptr);
    return &m_symtab->get(m_symidx);
}


Symbol* SymbolPointer::operator->()
{
    assert(m_symtab != nullptr);
    return &m_symtab->get(m_symidx);
}


FunctionScope& SymbolPointer::get_scope(const FunctionScope& hier) const
{
    auto& sym = m_symtab->get(m_symidx);
    assert(sym.type() == Symbol::Function);
    auto* parent_scope = hier.find_parent_scope(m_symtab);
    if (!parent_scope) {
        // use symtab's (generic) scope
        return m_symtab->scope()->get_subscope(sym.index());
    }
    return hier.find_parent_scope(m_symtab)->get_subscope(sym.index());

//    assert(sym.type() == Symbol::Function);
//    assert(m_symtab->module() != nullptr);
//    assert(sym.index() != no_index);
//    return m_symtab->module()->get_scope(sym.index());
}


FunctionScope& SymbolPointer::get_generic_scope() const
{
    auto& sym = m_symtab->get(m_symidx);
    assert(sym.type() == Symbol::Function);
    return m_symtab->scope()->get_subscope(sym.index());

//    assert(sym.type() == Symbol::Function);
//    assert(m_symtab->module() != nullptr);
//    assert(sym.index() != no_index);
//    return m_symtab->module()->get_scope(sym.index());
}


Index SymbolPointer::get_scope_index(const FunctionScope& hier) const
{
    auto& sym = m_symtab->get(m_symidx);
    assert(sym.type() == Symbol::Function);
    auto* parent_scope = hier.find_parent_scope(m_symtab);
    if (!parent_scope) {
        // use symtab's (generic) scope
        return m_symtab->scope()->get_subscope_index(sym.index());
    }
    return hier.find_parent_scope(m_symtab)->get_subscope_index(sym.index());

//    assert(sym.type() == Symbol::Function);
//    assert(sym.index() != no_index);
//    return sym.index();
}


Index SymbolPointer::get_generic_scope_index() const
{
    auto& sym = m_symtab->get(m_symidx);
    assert(sym.type() == Symbol::Function);
    assert(m_symtab->scope() != nullptr);
    return m_symtab->scope()->get_subscope_index(sym.index());

//    assert(sym.type() == Symbol::Function);
//    assert(sym.index() != no_index);
//    return sym.index();
}


Function& SymbolPointer::get_function(const FunctionScope& hier) const
{
    return get_scope(hier).function();
}


Class& SymbolPointer::get_class() const
{
    auto& sym = m_symtab->get(m_symidx);
    assert(sym.type() == Symbol::Class || sym.type() == Symbol::Method);
    assert(m_symtab->module() != nullptr);
    assert(sym.index() != no_index);
    return m_symtab->module()->get_class(sym.index());
}


//------------------------------------------------------------------------------


SymbolTable::SymbolTable(std::string name, SymbolTable* parent)
        : m_name(std::move(name)), m_parent(parent),
          m_module(parent ? parent->module() : nullptr)
{}


std::string SymbolTable::qualified_name() const
{
    std::string result = m_name;
    const auto* parent = m_parent;
    while (parent != nullptr) {
        result = parent->name() + "::" + result;
        parent = parent->parent();
    }
    return result;
}


SymbolPointer SymbolTable::add(Symbol&& symbol)
{
    // deduplicate StructItem symbols
    if (symbol.type() == Symbol::StructItem) {
        auto sym_ptr = find(symbol);
        if (sym_ptr) {
            assert(sym_ptr.symtab() == this);
            return sym_ptr;
        }
    }

    m_symbols.emplace_back(std::move(symbol));
    return {*this, Index(m_symbols.size() - 1)};
}


SymbolTable& SymbolTable::add_child(const std::string& name)
{
    m_children.emplace_back(name, this);
    return m_children.back();
}


unsigned SymbolTable::depth(const SymbolTable* p_symtab) const
{
    unsigned res = 0;
    const auto* p = this;
    while (p != p_symtab) {
        ++res;
        p = p->parent();
    }
    return res;
}


Size SymbolTable::count(Symbol::Type type) const
{
    return std::count_if(m_symbols.begin(), m_symbols.end(),
            [type](const Symbol& sym) { return sym.type() == type; });
}


Symbol& SymbolTable::get(Index idx)
{
    assert(idx < m_symbols.size());
    return m_symbols[idx];
}


const Symbol& SymbolTable::get(Index idx) const
{
    assert(idx < m_symbols.size());
    return m_symbols[idx];
}


SymbolPointer SymbolTable::find(const Symbol& symbol)
{
    auto it = std::find_if(m_symbols.begin(), m_symbols.end(),
                           [&symbol](const Symbol& other){ return symbol == other; });
    if (it == m_symbols.end())
        return {*this, no_index};
    return {*this, Index(it - m_symbols.begin())};
}


SymbolPointer SymbolTable::find_by_name(std::string_view name)
{
    auto it = std::find_if(m_symbols.rbegin(), m_symbols.rend(),
            [&name](const Symbol& sym){ return sym.name() == name; });
    if (it == m_symbols.rend())
        return {*this, no_index};
    return {*this, Index((m_symbols.rend() - it) - 1)};
}


SymbolPointer SymbolTable::find_by_index(Symbol::Type type, Index index)
{
    auto it = std::find_if(m_symbols.rbegin(), m_symbols.rend(),
                           [type,index](const Symbol& sym)
                           { return sym.type() == type && sym.index() == index; });
    if (it == m_symbols.rend())
        return {*this, no_index};
    return {*this, Index((m_symbols.rend() - it) - 1)};
}


SymbolPointer SymbolTable::find_last_of(const std::string& name,
                                        Symbol::Type type)
{
    auto it = std::find_if(m_symbols.rbegin(), m_symbols.rend(),
                   [&name, type](const Symbol& sym) {
                        return sym.type() == type && sym.name() == name;
                   });
    if (it == m_symbols.rend())
        return {*this, no_index};
    return {*this, Index((m_symbols.rend() - it) - 1)};
}


SymbolPointer SymbolTable::find_last_of(Symbol::Type type)
{
    auto it = std::find_if(m_symbols.rbegin(), m_symbols.rend(),
            [type](const Symbol& sym) { return sym.type() == type; });
    if (it == m_symbols.rend())
        return {*this, no_index};
    return {*this, Index((m_symbols.rend() - it) - 1)};
}


SymbolPointerList SymbolTable::filter(const std::string& name, Symbol::Type type)
{
    SymbolPointerList res;
    Index i = 0;
    for (const auto& sym : m_symbols) {
        if (sym.type() == type && sym.name() == name)
            res.emplace_back(*this, i);
        ++i;
    }
    return res;
}


SymbolTable* SymbolTable::find_child_by_name(std::string_view name)
{
    auto it = std::find_if(m_children.begin(), m_children.end(),
            [&name](const SymbolTable& s){ return s.name() == name; });
    if (it == m_children.end())
        return nullptr;
    return &*it;
}


} // namespace xci::script
