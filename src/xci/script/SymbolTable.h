// SymbolTable.h created on 2019-07-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_SYMBOL_TABLE_H
#define XCI_SCRIPT_SYMBOL_TABLE_H

#include <xci/core/container/ChunkedStack.h>
#include <xci/core/mixin.h>
#include <vector>
#include <string>
#include <string_view>

namespace xci::script {

class Symbol;
class SymbolTable;
class Function;
class Class;
class Module;


using Index = uint32_t;
static constexpr Index no_index {~0u};

using Size = uint32_t;


class SymbolPointer {
public:
    SymbolPointer() = default;
    SymbolPointer(SymbolTable& symtab, Index idx) : m_symtab(&symtab), m_symidx(idx) {}

    explicit operator bool() const { return m_symtab != nullptr && m_symidx != no_index; }

    const Symbol& operator*() const;
    const Symbol* operator->() const;
    Symbol* operator->();

    Function& get_function() const;
    Class& get_class() const;

    SymbolTable* symtab() const { return m_symtab; }
    Index symidx() const { return m_symidx; }

    bool operator==(const SymbolPointer& rhs) const = default;
    bool operator<(const SymbolPointer& rhs) const {
        return std::tie(m_symtab, m_symidx) < std::tie(rhs.m_symtab, rhs.m_symidx);
    }

private:
    SymbolTable* m_symtab = nullptr;   // owning table
    Index m_symidx = no_index;         // index of item in the table
};


class Symbol {
public:
    enum Type {
        Unresolved,

        // module-level
        Module,             // imported module
        Function,           // static function
        Value,              // static value
        TypeName,           // type information (index = type index in module)
        Class,              // type class (index = class index in module)
        Instance,           // instance of type class (index = instance index in module)
        Method,             // method declaration (index = class index, ref = symbol in class scope)

        // function scope
        Parameter,          // function parameter
        Nonlocal,           // non-local parameter, i.e. a capture from outer scope
        Instruction,        // intrinsics resolve to this, the index is Opcode
        TypeVar,            // type variable in generic function (index = var ID)

        // struct type
        StructItem,         // name = item name, index = struct type index in module

        // special
        TypeId,             // translate type name to type ID (index = type index in builtin if < 32, else type index in current module + 32)
    };

    Symbol() = default;  // only for deserialization
    explicit Symbol(std::string name) : m_name(std::move(name)) {}
    Symbol(std::string name, Type type) : m_name(std::move(name)), m_type(type) {}
    Symbol(std::string name, Type type, Index idx) : m_name(std::move(name)), m_type(type), m_index(idx) {}
    Symbol(std::string name, Type type, Index idx, size_t depth)
        : m_name(std::move(name)), m_type(type), m_index(idx), m_depth(depth) {}
    Symbol(const SymbolPointer& ref, Type type)
            : m_name(ref->name()), m_type(type), m_ref(ref) {}
    Symbol(const SymbolPointer& ref, Type type, Index idx, size_t depth)
            : m_name(ref->name()), m_type(type), m_index(idx),
              m_depth(depth), m_ref(ref) {}

    bool operator==(const Symbol&) const = default;

    const std::string& name() const { return m_name; }
    Type type() const { return m_type; }
    Index index() const { return m_index; }
    size_t depth() const { return m_depth; }
    SymbolPointer ref() const { return m_ref; }
    // in case of overloaded function, this points to next overload
    SymbolPointer next() const { return m_next; }
    bool is_callable() const { return m_is_callable; }
    bool is_defined() const { return m_is_defined; }

    Symbol& set_type(Type type) { m_type = type; return *this; }
    Symbol& set_index(Index idx) { m_index = idx; return *this; }
    Symbol& set_depth(size_t depth) { m_depth = depth; return *this; }
    Symbol& set_ref(const SymbolPointer& ref) { m_ref = ref; return *this; }
    Symbol& set_next(const SymbolPointer& next) { m_next = next; return *this; }
    Symbol& set_callable(bool callable) { m_is_callable = callable; return *this; }
    Symbol& set_defined(bool defined) { m_is_defined = defined; return *this; }

    template<class Archive>
    void save(Archive& ar) const {
        uint8_t flags = uint8_t(m_is_callable) | uint8_t(unsigned(m_is_defined) << 1);
        ar ("name", m_name) ("type", m_type) ("index", m_index) ("depth", m_depth) ("flags", flags);
    }

    template<class Archive>
    void load(Archive& ar) {
        uint8_t flags = 0;
        ar(m_name)(m_type)(m_index)(m_depth)(flags);
        m_is_callable = bool(flags & 0x01);
        m_is_defined = bool(flags & 0x02);
    }

private:
    std::string m_name;
    Type m_type = Unresolved;
    Index m_index = no_index;
    size_t m_depth = 0;  // 1 = parent, 2 = parent of parent, ...
    SymbolPointer m_ref;
    SymbolPointer m_next;
    bool m_is_callable: 1 = false;
    bool m_is_defined: 1 = false;  // only declared / already defined
};


/// Hierarchical symbol table
///
/// Symbol indices are persistent - symbols can only be added, never removed
/// If a symbol does not appear in optimized code, it is marked as such.
/// Count and indexes of actual local variables are computed from symbol table
/// (by skipping unused symbols).

class SymbolTable: private core::NonCopyable {
public:
    SymbolTable() = default;
    explicit SymbolTable(std::string name, SymbolTable* parent = nullptr);

    void set_name(const std::string& name) { m_name = name; }
    const std::string& name() const { return m_name; }
    std::string qualified_name() const;

    SymbolTable& add_child(const std::string& name);
    SymbolTable* parent() const { return m_parent; }
    unsigned level() const { return depth(nullptr) - 1; }  // number of parents above this symtab
    unsigned depth(const SymbolTable* p_symtab) const;  // number of parents up to `p_symtab`

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
    SymbolPointer find(const Symbol& symbol);
    SymbolPointer find_by_name(std::string_view name);
    SymbolPointer find_last_of(const std::string& name, Symbol::Type type);
    SymbolPointer find_last_of(Symbol::Type type);

    Size count(Symbol::Type type) const;
    void update_nonlocal_indices();

    /// Check symbol table for overloaded function name
    /// and connect the symbols using `next` pointer
    /// (making the overloads visible to SymbolResolver)
    void detect_overloads(const std::string& name);

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

        using const_iterator = typename core::ChunkedStack<SymbolTable>::const_iterator;
        const_iterator begin() const { return m_symtab.m_children.cbegin(); }
        const_iterator end() const { return m_symtab.m_children.cend(); }

    private:
        const SymbolTable& m_symtab;
    };

    Children children() const { return Children{*this}; }
    SymbolTable* find_child_by_name(std::string_view name);

    template<class Archive>
    void save(Archive& ar) const {
        ar ("name", m_name) ("symbols", m_symbols) ("children", m_children);
    }

    template<class Archive>
    void load(Archive& ar) {
        ar(m_name)(m_symbols)(m_children);
        for (SymbolTable& child : m_children) {
            child.m_parent = this;
        }
    }

private:
    std::string m_name;
    SymbolTable* m_parent = nullptr;
    Function* m_function = nullptr;
    Class* m_class = nullptr;
    Module* m_module = nullptr;
    core::ChunkedStack<SymbolTable> m_children;  // NOTE: member addresses must not change
    std::vector<Symbol> m_symbols;
};


} // namespace xci::script

#endif // include guard
