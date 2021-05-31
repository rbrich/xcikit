// SymbolTable.cpp created on 2019-07-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "SymbolTable.h"
#include "Module.h"
#include <cassert>
#include <algorithm>

namespace xci::script {


SymbolTable::SymbolTable(std::string name, SymbolTable* parent)
        : m_name(std::move(name)), m_parent(parent),
          m_module(parent ? parent->module() : nullptr)
{}


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


Function& SymbolPointer::get_function()
{
    auto& sym = m_symtab->get(m_symidx);
    assert(sym.type() == Symbol::Function);
    assert(m_symtab->module() != nullptr);
    return m_symtab->module()->get_function(sym.index());
}


SymbolPointer SymbolTable::add(Symbol&& symbol)
{
    m_symbols.emplace_back(std::move(symbol));
    return {*this, m_symbols.size() - 1};
}


SymbolTable& SymbolTable::add_child(const std::string& name)
{
    m_children.emplace_back(name, this);
    return m_children.back();
}


unsigned SymbolTable::level() const
{
    unsigned res = 0;
    const auto* p = m_parent;
    while (p != nullptr) {
        ++res;
        p = p->parent();
    }
    return res;
}


size_t SymbolTable::count(Symbol::Type type) const
{
    return std::count_if(m_symbols.begin(), m_symbols.end(),
            [type](const Symbol& sym) { return sym.type() == type; });
}


void SymbolTable::update_nonlocal_indices()
{
    size_t idx = 0;
    for (auto& sym : m_symbols) {
        if (sym.type() == Symbol::Nonlocal) {
            sym.set_index(idx++);
        }
    }
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


SymbolPointer SymbolTable::find_by_name(const std::string& name)
{
    auto it = std::find_if(m_symbols.begin(), m_symbols.end(),
            [&name](const Symbol& sym){ return sym.name() == name; });
    if (it == m_symbols.end())
        return {*this, no_index};
    return {*this, Index(it - m_symbols.begin())};
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


void SymbolTable::detect_overloads(const std::string& name)
{
    Symbol* prev = nullptr;
    for (size_t i = 0; i != m_symbols.size(); ++i) {
        if (m_symbols[i].name() == name) {
            if (prev) {
                prev->set_next({*this, i});
            }
            prev = &m_symbols[i];
        }
    }
}


} // namespace xci::script
