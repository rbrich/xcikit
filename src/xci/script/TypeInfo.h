// TypeInfo.h created on 2019-06-09, part of XCI toolkit
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

#ifndef XCI_SCRIPT_TYPEINFO_H
#define XCI_SCRIPT_TYPEINFO_H

#include <cstdint>
#include <ostream>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <utility>

namespace xci::script {


enum class Type : uint8_t {
    Unknown,    // type not known at this time (might be inferred or generic)
    Void,       // void type - has no value

    // Plain types
    Bool,
    Byte,       // uint8
    Char,       // Unicode codepoint (char32)
    Int32,
    Int64,
    Float32,
    Float64,

    // Complex types
    String,     // special kind of list, behaves like [Char] but is compressed (UTF-8)
    List,       // list of same element type (elem type is part of type, size is part of value)
    Tuple,      // tuple of different value types
    Function,   // function type, has signature (parameters, return type) and code
    Module,     // module type, carries global names, constants, functions
    //Data,       // user defined data type
};


struct Signature;

class TypeInfo {
public:
    TypeInfo() = default;
    explicit TypeInfo(Type type) : m_type(type) {}
    explicit TypeInfo(Type type, uint8_t var) : m_type(type), m_var(var) {}
    // Function
    explicit TypeInfo(std::shared_ptr<Signature> signature)
        : m_type(Type::Function), m_signature(std::move(signature)) {}
    // Tuple
    explicit TypeInfo(std::vector<TypeInfo> subtypes)
        : m_type(Type::Tuple), m_subtypes(std::move(subtypes)) {}
    // List
    explicit TypeInfo(Type type, TypeInfo elem_type)
        : m_type(type), m_subtypes({std::move(elem_type)}) {}

    size_t size() const;
    void foreach_heap_slot(std::function<void(size_t offset)> cb) const;

    Type type() const { return m_type; }
    bool is_callable() const { return type() == Type::Function; }

    bool is_unknown() const { return m_type == Type::Unknown; }
    uint8_t generic_var() const { return m_var; }
    void replace_var(uint8_t idx, const TypeInfo& ti);

    Signature& signature() { return *m_signature; }
    const Signature& signature() const { return *m_signature; }
    std::shared_ptr<Signature> signature_ptr() const { return m_signature; }

    TypeInfo effective_type() const;

    const std::vector<TypeInfo>& subtypes() const { return m_subtypes; }
    const TypeInfo& elem_type() const;

    bool operator==(const TypeInfo& rhs) const;
    bool operator!=(const TypeInfo& rhs) const { return !(*this == rhs); }

    explicit operator bool() const { return m_type != Type::Unknown; }

private:
    Type m_type { Type::Unknown };
    uint8_t m_var {0};  // for unknown type, specifies which type variable this represents
    std::shared_ptr<Signature> m_signature;
    std::vector<TypeInfo> m_subtypes;
};


struct Signature {
    std::vector<TypeInfo> nonlocals;
    std::vector<TypeInfo> partial;
    std::vector<TypeInfo> params;
    TypeInfo return_type;

    void add_nonlocal(TypeInfo&& ti) { nonlocals.emplace_back(ti); }
    void add_partial(TypeInfo&& ti) { partial.emplace_back(ti); }
    void add_parameter(TypeInfo&& ti) { params.emplace_back(ti); }
    void set_return_type(TypeInfo ti) { return_type = std::move(ti); }

    // Check return type matches and set it to concrete type if it's generic.
    void resolve_return_type(const TypeInfo& t);

    bool operator==(const Signature& rhs) const {
        return params == rhs.params
            && partial == rhs.partial
            && nonlocals == rhs.nonlocals
            && return_type == rhs.return_type;
    }
    bool operator!=(const Signature& rhs) const { return !(rhs == *this); }
};


} // namespace xci::script

#endif // include guard
