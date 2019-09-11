// SymbolTable.cpp created on 2019-07-14, part of XCI toolkit
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

#include "SymbolTable.h"
#include <cassert>
#include <algorithm>
#include <ostream>
#include <iomanip>

namespace xci::script {

using namespace std;


Symbol& SymbolPointer::operator*()
{
    assert(m_symtab != nullptr);
    return m_symtab->get(m_index);
}


const Symbol& SymbolPointer::operator*() const
{
    assert(m_symtab != nullptr);
    return m_symtab->get(m_index);
}


Symbol* SymbolPointer::operator->()
{
    assert(m_symtab != nullptr);
    return &m_symtab->get(m_index);
}


const Symbol* SymbolPointer::operator->() const
{
    assert(m_symtab != nullptr);
    return &m_symtab->get(m_index);
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


size_t SymbolTable::count_nonlocals() const
{
    return std::count_if(m_symbols.begin(), m_symbols.end(),
            [](const Symbol& sym) { return sym.type() == Symbol::Nonlocal; });
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


struct StreamOptions {
    unsigned level;
};

static StreamOptions& stream_options(std::ostream& os) {
    static int idx = std::ios_base::xalloc();
    return reinterpret_cast<StreamOptions&>(os.iword(idx));
}

static std::ostream& put_indent(std::ostream& os)
{
    std::string pad(stream_options(os).level * 3, ' ');
    return os << pad;
}

static std::ostream& more_indent(std::ostream& os)
{
    stream_options(os).level += 1;
    return os;
}

static std::ostream& less_indent(std::ostream& os)
{
    assert(stream_options(os).level >= 1);
    stream_options(os).level -= 1;
    return os;
}


std::ostream& operator<<(std::ostream& os, Symbol::Type v)
{
    switch (v) {
        case Symbol::Unresolved:    return os << "Unresolved";
        case Symbol::Value:         return os << "Value";
        case Symbol::Parameter:     return os << "Parameter";
        case Symbol::Nonlocal:      return os << "Nonlocal";
        case Symbol::Function:      return os << "Function";
        case Symbol::Module:        return os << "Module";
        case Symbol::Instruction:   return os << "Instruction";
        case Symbol::Class:         return os << "Class";
        case Symbol::TypeVar:       return os << "TypeVar";
    }
    return os;
}


std::ostream& operator<<(std::ostream& os, const SymbolPointer& v)
{
    os << v->type();
    if (v->index() != no_index)
        os << " #" << v->index();
    if (v.symtab() != nullptr)
        os << " @" << std::hex << v.symtab() << std::dec;
    if (v->ref())
        os << " -> " << v->ref();
    return os;
}


std::ostream& operator<<(std::ostream& os, const Symbol& v)
{
    os << left << setw(20) << v.name() << " "
       << left << setw(18) << v.type();
    if (v.index() != no_index)
        os << " #" << v.index();
    if (v.ref())
        os << " -> " << v.ref()->type() << " #" << v.ref()->index();
    if (v.depth() != 0)
        os << ", depth -" << v.depth();
    return os;
}


std::ostream& operator<<(std::ostream& os, const SymbolTable& v)
{
    os << put_indent << "--- " << v.name() << " ---" << endl;
    for (const auto& sym : v) {
        os << put_indent << sym << endl;
    }

    os << more_indent;
    for (const auto& child : v.children()) {
        os << child << endl;
    }
    os << less_indent;
    return os;
}


} // namespace xci::script
