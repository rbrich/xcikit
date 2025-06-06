// SymbolTable.cpp created on 2019-07-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "SymbolTable.h"
#include "Module.h"
#include "TypeInfo.h"

#include <cassert>
#include <ranges>
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


Scope& SymbolPointer::get_scope(const Scope& hier) const
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


Scope& SymbolPointer::get_generic_scope() const
{
    auto& sym = m_symtab->get(m_symidx);
    assert(sym.type() == Symbol::Function);
    return m_symtab->scope()->get_subscope(sym.index());

//    assert(sym.type() == Symbol::Function);
//    assert(m_symtab->module() != nullptr);
//    assert(sym.index() != no_index);
//    return m_symtab->module()->get_scope(sym.index());
}


Index SymbolPointer::get_scope_index(const Scope& hier) const
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


Function& SymbolPointer::get_function(const Scope& hier) const
{
    return get_scope(hier).function();
}


const TypeInfo& SymbolPointer::get_type() const
{
    auto& sym = m_symtab->get(m_symidx);
    assert(sym.type() == Symbol::TypeName);
    assert(m_symtab->module() != nullptr);
    return m_symtab->module()->get_type(sym.index());
}


Class& SymbolPointer::get_class() const
{
    auto& sym = m_symtab->get(m_symidx);
    assert(sym.type() == Symbol::Class || sym.type() == Symbol::Method);
    assert(m_symtab->module() != nullptr);
    assert(sym.index() != no_index);
    return m_symtab->module()->get_class(sym.index());
}


Module& SymbolPointer::get_module() const
{
    auto& sym = m_symtab->get(m_symidx);
    assert(sym.type() == Symbol::Module);
    assert(m_symtab->module() != nullptr);
    assert(sym.index() != no_index);
    return m_symtab->module()->get_imported_module(sym.index());
}


std::string SymbolPointer::symtab_qualified_name() const
{
    return m_symtab ? m_symtab->qualified_name() : "";
}


//------------------------------------------------------------------------------


SymbolTable::SymbolTable(NameId name, SymbolTable* parent)
        : m_name(name), m_parent(parent),
          m_module(parent ? parent->module() : nullptr)
{}


std::string SymbolTable::qualified_name() const
{
    std::string result = name().str();
    const auto* parent = m_parent;
    while (parent != nullptr) {
        result = parent->name().str() + "::" + result;
        parent = parent->parent();
    }
    return result;
}


SymbolPointer SymbolTable::add(const Symbol& symbol)
{
    m_symbols.push_back(symbol);
    return {*this, Index(m_symbols.size() - 1)};
}


SymbolTable& SymbolTable::add_child(NameId name)
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
    const auto it = std::ranges::find_if(m_symbols,
                           [&symbol](const Symbol& other){ return symbol == other; });
    if (it == m_symbols.end())
        return {*this, no_index};
    return {*this, Index(it - m_symbols.begin())};
}


SymbolPointer SymbolTable::find_by_name(NameId name)
{
    const auto it = std::ranges::find_if(std::ranges::reverse_view(m_symbols),
            [name](const Symbol& sym){ return sym.name() == name; });
    if (it == m_symbols.rend())
        return {*this, no_index};
    return {*this, Index((m_symbols.rend() - it) - 1)};
}


SymbolPointer SymbolTable::find_by_index(Symbol::Type type, Index index)
{
    const auto it = std::ranges::find_if(std::ranges::reverse_view(m_symbols),
                           [type,index](const Symbol& sym)
                           { return sym.type() == type && sym.index() == index; });
    if (it == m_symbols.rend())
        return {*this, no_index};
    return {*this, Index((m_symbols.rend() - it) - 1)};
}


SymbolPointer SymbolTable::find_last_of(NameId name,
                                        Symbol::Type type)
{
    const auto it = std::ranges::find_if(std::ranges::reverse_view(m_symbols),
                   [name, type](const Symbol& sym) {
                        return sym.type() == type && sym.name() == name;
                   });
    if (it == m_symbols.rend())
        return {*this, no_index};
    return {*this, Index((m_symbols.rend() - it) - 1)};
}


SymbolPointer SymbolTable::find_last_of(Symbol::Type type)
{
    const auto it = std::ranges::find_if(std::ranges::reverse_view(m_symbols),
            [type](const Symbol& sym) { return sym.type() == type; });
    if (it == m_symbols.rend())
        return {*this, no_index};
    return {*this, Index((m_symbols.rend() - it) - 1)};
}


SymbolPointerList SymbolTable::filter(Symbol::Type type)
{
    SymbolPointerList res;
    Index i = 0;
    for (const auto& sym : m_symbols) {
        if (sym.type() == type)
            res.emplace_back(*this, i);
        ++i;
    }
    return res;
}


SymbolPointerList SymbolTable::filter(NameId name, Symbol::Type type)
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


SymbolTable* SymbolTable::find_child_by_name(NameId name)
{
    const auto it = std::ranges::find_if(m_children,
            [name](const SymbolTable& s){ return s.name() == name; });
    if (it == m_children.end())
        return nullptr;
    return &*it;
}


} // namespace xci::script
