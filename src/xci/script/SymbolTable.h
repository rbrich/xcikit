#include <utility>

// SymbolTable.h created on 2019-07-14, part of XCI toolkit
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

#ifndef XCI_SCRIPT_SYMBOLTABLE_H
#define XCI_SCRIPT_SYMBOLTABLE_H

#include <vector>
#include <list>
#include <string>

namespace xci::script {


class Symbol;
class SymbolTable;
class Function;
class Class;
class Module;


using Index = size_t;
static constexpr Index no_index {SIZE_MAX};

class SymbolPointer {
public:
    SymbolPointer() = default;
    SymbolPointer(SymbolTable& symtab, Index idx) : m_symtab(&symtab), m_index(idx) {}

    explicit operator bool() const { return m_symtab != nullptr && m_index != no_index; }

    SymbolPointer& operator= (const SymbolPointer& rhs) = default;

    Symbol& operator* ();
    const Symbol& operator*() const;

    Symbol* operator-> ();
    const Symbol* operator-> () const;

    SymbolTable* symtab() const { return m_symtab; }
    Index symidx() const { return m_index; }

    bool operator==(const SymbolPointer& rhs) const {
        return m_symtab == rhs.m_symtab && m_index == rhs.m_index;
    }

private:
    SymbolTable* m_symtab = nullptr;  // owning table
    Index m_index = no_index;         // index of item in the table
};


class Symbol {
public:
    enum Type {
        Unresolved,
        Value,              // either local value in function scope or static value (module-level)
        Parameter,          // function parameter in function scope
        Nonlocal,           // non-local value in function scope, i.e. a capture from outer scope
        Function,           // static function (module-level)
        Module,             // imported module (module-level)
        Instruction,        // intrinsics (e.g. __equal_32) resolve to this, index is Opcode
        Class,              // type class
        Method,             // method declaration: index = class index, ref = symbol in class scope
        Instance,           // instance of type class
        TypeName,           // type
        TypeVar,            // type variable
    };

    explicit Symbol(std::string name) : m_name(std::move(name)) {}
    Symbol(std::string name, Index idx) : m_name(std::move(name)), m_type(Value), m_index(idx) {}
    Symbol(std::string name, Type type) : m_name(std::move(name)), m_type(type) {}
    Symbol(std::string name, Type type, Index idx) : m_name(std::move(name)), m_type(type), m_index(idx) {}
    Symbol(std::string name, Type type, Index idx, size_t depth)
        : m_name(std::move(name)), m_type(type), m_index(idx), m_depth(depth) {}
    Symbol(const SymbolPointer& ref, Type type)
            : m_name(ref->name()), m_type(type), m_ref(ref) {}
    Symbol(const SymbolPointer& ref, Type type, size_t depth)
        : m_name(ref->name()), m_type(type), m_index(ref.symidx()),
          m_depth(depth), m_ref(ref) {}

    const std::string& name() const { return m_name; }
    Type type() const { return m_type; }
    Index index() const { return m_index; }
    size_t depth() const { return m_depth; }
    SymbolPointer ref() const { return m_ref; }
    // in case of overloaded function, this points to next overload
    SymbolPointer next() const { return m_next; }
    bool is_callable() const { return m_is_callable; }

    void set_type(Type type) { m_type = type; }
    void set_index(Index idx) { m_index = idx; }
    void set_depth(size_t depth) { m_depth = depth; }
    void set_ref(const SymbolPointer& ref) { m_ref = ref; }
    void set_next(const SymbolPointer& next) { m_next = next; }
    void set_callable(bool callable) { m_is_callable = callable; }

private:
    std::string m_name;
    Type m_type = Unresolved;
    Index m_index = no_index;
    size_t m_depth = 0;  // 1 = parent, 2 = parent of parent, ...
    SymbolPointer m_ref;
    SymbolPointer m_next;
    bool m_is_callable = false;
};


/// Hierarchical symbol table
///
/// Symbol indices are persistent - symbols can only be added, never removed
/// If a symbol does not appear in optimized code, it is marked as such.
/// Count and indexes of actual local variables are computed from symbol table
/// (by skipping unused symbols).

class SymbolTable {
public:
    SymbolTable() = default;
    explicit SymbolTable(std::string name, SymbolTable* parent = nullptr)
        : m_name(std::move(name)), m_parent(parent) {}

    const std::string& name() const { return m_name; }

    SymbolTable& add_child(const std::string& name);
    SymbolTable* parent() const { return m_parent; }

    // related function
    void set_function(Function* function) { m_function = function; }
    Function* function() const { return m_function; }

    // related class
    void set_class(Class* cls) { m_class = cls; }
    Class* class_() const { return m_class; }

    // related module
    void set_module(Module* module) { m_module = module; }
    Module* module() const { return m_module; }

    SymbolPointer add(Symbol&& symbol);

    Symbol& get(Index idx);
    const Symbol& get(Index idx) const;

    // find symbol in this table
    SymbolPointer find_by_name(const std::string& name);
    SymbolPointer find_last_of(const std::string& name, Symbol::Type type);

    // return actual number of nonlocals (skipping unreferenced symbols)
    size_t count_nonlocals() const;
    void update_nonlocal_indices();

    // FIXME: use Pointer<T> / ConstPointer<T> directly as iterator
    using const_iterator = typename std::vector<Symbol>::const_iterator;
    const_iterator begin() const { return m_symbols.begin(); }
    const_iterator end() const { return m_symbols.end(); }

    using iterator = typename std::vector<Symbol>::iterator;
    iterator begin() { return m_symbols.begin(); }
    iterator end() { return m_symbols.end(); }

    using const_reverse_iterator = typename std::vector<Symbol>::const_reverse_iterator;
    const_reverse_iterator rbegin() const { return m_symbols.rbegin(); }
    const_reverse_iterator rend() const { return m_symbols.rend(); }

    using reverse_iterator = typename std::vector<Symbol>::reverse_iterator;
    reverse_iterator rbegin() { return m_symbols.rbegin(); }
    reverse_iterator rend() { return m_symbols.rend(); }

    bool empty() const { return m_symbols.empty(); }
    size_t size() const { return m_symbols.size(); }

    class Children {
    public:
        explicit Children(const SymbolTable& symtab) : m_symtab(symtab) {}

        using const_iterator = typename std::list<SymbolTable>::const_iterator;
        const_iterator begin() const { return m_symtab.m_children.begin(); }
        const_iterator end() const { return m_symtab.m_children.end(); }

    private:
        const SymbolTable& m_symtab;
    };

    Children children() const { return Children{*this}; }

private:
    std::string m_name;   // only for debugging
    SymbolTable* m_parent = nullptr;
    Function* m_function = nullptr;
    Class* m_class = nullptr;
    Module* m_module = nullptr;
    std::list<SymbolTable> m_children;  // NOTE: member addresses must not change
    std::vector<Symbol> m_symbols;
};


} // namespace xci::script

#endif // include guard
