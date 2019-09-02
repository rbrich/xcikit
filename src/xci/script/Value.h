// Value.h created on 2019-05-18, part of XCI toolkit
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

#ifndef XCI_SCRIPT_VALUE_H
#define XCI_SCRIPT_VALUE_H

#include "TypeInfo.h"
#include "Heap.h"
#include <xci/compat/utility.h>

#include <ostream>
#include <utility>
#include <vector>
#include <map>
#include <string_view>
#include <cassert>
#include <cstring>
#include <cstdint>

namespace xci::script {

class Values;
class Function;
class Module;


namespace value {

class Void;
class Bool;
class Byte;
class Char;
class Int32;
class Int64;
class Float32;
class Float64;
class String;
class List;
class Tuple;
class Lambda;
class Module;

class Visitor {
public:
    virtual void visit(const Void&) = 0;
    virtual void visit(const Bool&) = 0;
    virtual void visit(const Byte&) = 0;
    virtual void visit(const Char&) = 0;
    virtual void visit(const Int32&) = 0;
    virtual void visit(const Int64&) = 0;
    virtual void visit(const Float32&) = 0;
    virtual void visit(const Float64&) = 0;
    virtual void visit(const String&) = 0;
    virtual void visit(const List&) = 0;
    virtual void visit(const Tuple&) = 0;
    virtual void visit(const Lambda&) = 0;
    virtual void visit(const Module&) = 0;
};

} // namespace value


class Value {
public:
    static std::unique_ptr<Value> create(const TypeInfo& type_info);

    virtual ~Value() = default;

    virtual std::unique_ptr<Value> make_copy() const = 0;

    // Serialize/deserialize the value
    // Note that if the value lives on Heap, this also affects refcount
    // To read value and also keep it in original location,
    // you MUST also call incref.
    virtual void write(byte* buffer) const = 0;
    virtual void read(const byte* buffer) = 0;
    virtual void incref() const {}
    virtual void decref() const {}

    size_t size() const { return type_info().size(); }

    virtual void apply(value::Visitor& visitor) const = 0;

    virtual TypeInfo type_info() const = 0;
    Type type() const { return type_info().type(); }
    bool is_void() const { return type_info().type() == Type::Void; }
    bool is_bool() const { return type_info().type() == Type::Bool; }
    bool is_callable() const { return type_info().type() == Type::Function; }
};


class Values {
public:
    Values() = default;
    Values(Values&&) = default;
    Values(const Values& other);
    Values& operator =(const Values& rhs);
    Values& operator =(Values&& rhs) noexcept = default;

    // reserve number of values
    void reserve(size_t n) { m_items.reserve(n); }
    // get number of values
    size_t size() const { return m_items.size(); }
    bool empty() const { return m_items.empty(); }

    // get size of all values in bytes
    size_t raw_size() const;

    void add(std::unique_ptr<Value>&& value) { m_items.emplace_back(std::forward<std::unique_ptr<Value>>(value)); }

    std::unique_ptr<Value>& operator[](size_t i) { return m_items[i]; }
    const Value& operator[](size_t i) const { return *m_items[i]; }

    bool operator==(const Values& rhs) const { return m_items == rhs.m_items; }
    bool operator!=(const Values& rhs) const { return m_items != rhs.m_items; }

    using const_iterator = std::vector<std::unique_ptr<Value>>::const_iterator;
    const_iterator begin() const { return m_items.begin(); }
    const_iterator end() const { return m_items.end(); }
    const std::unique_ptr<Value>& back() const { return m_items.back(); }

private:
    std::vector<std::unique_ptr<Value>> m_items;
};


namespace value {


// Empty value, doesn't carry any information
// But it still needs to have some size, so it's written to stack as single 0 byte
class Void: public Value {
public:
    std::unique_ptr<Value> make_copy() const override { return std::make_unique<Void>(); }
    void write(byte* buffer) const override { *buffer = byte{0}; }
    void read(const byte* buffer) override {}
    TypeInfo type_info() const override { return TypeInfo{Type::Void}; }
    void apply(value::Visitor& visitor) const override { visitor.visit(*this); }
};


// ----------- //
// Plain types //
// ----------- //


class Bool: public Value {
public:
    Bool() : m_value(false) {}
    explicit Bool(bool v) : m_value(v) {}
    std::unique_ptr<Value> make_copy() const override { return std::make_unique<Bool>(m_value); }

    void write(byte* buffer) const override { *buffer = byte(m_value); }
    void read(const byte* buffer) override { m_value = bool(*buffer); }
    TypeInfo type_info() const override { return TypeInfo{Type::Bool}; }

    void apply(value::Visitor& visitor) const override { visitor.visit(*this); }

    bool value() const { return m_value; }

private:
    bool m_value;
};


class Byte: public Value {
public:
    Byte() : m_value(0) {}
    explicit Byte(uint8_t v) : m_value(v) {}
    std::unique_ptr<Value> make_copy() const override { return std::make_unique<Byte>(m_value); }

    void write(byte* buffer) const override { *buffer = byte(m_value); }
    void read(const byte* buffer) override { m_value = uint8_t(*buffer); }
    TypeInfo type_info() const override { return TypeInfo{Type::Byte}; }

    void apply(value::Visitor& visitor) const override { visitor.visit(*this); }

    uint8_t value() const { return m_value; }

private:
    uint8_t m_value;
};


class Char: public Value {
public:
    Char() : m_value(0) {}
    explicit Char(char32_t v) : m_value(v) {}
    std::unique_ptr<Value> make_copy() const override { return std::make_unique<Char>(m_value); }

    void write(byte* buffer) const override { std::memcpy(buffer, &m_value, sizeof(m_value)); }
    void read(const byte* buffer) override { std::memcpy(&m_value, buffer, sizeof(m_value)); }
    TypeInfo type_info() const override { return TypeInfo{Type::Char}; }

    void apply(value::Visitor& visitor) const override { visitor.visit(*this); }

    char32_t value() const { return m_value; }

private:
    char32_t m_value;
};


class Int32: public Value {
public:
    Int32() : m_value(0) {}
    explicit Int32(int32_t v) : m_value(v) {}
    std::unique_ptr<Value> make_copy() const override { return std::make_unique<Int32>(m_value); }

    void write(byte* buffer) const override { std::memcpy(buffer, &m_value, sizeof(m_value)); }
    void read(const byte* buffer) override { std::memcpy(&m_value, buffer, sizeof(m_value)); }
    TypeInfo type_info() const override { return TypeInfo{Type::Int32}; }

    void apply(value::Visitor& visitor) const override { visitor.visit(*this); }

    int32_t value() const { return m_value; }

private:
    int32_t m_value;
};


class Int64: public Value {
public:
    Int64() : m_value(0) {}
    explicit Int64(int64_t v) : m_value(v) {}
    std::unique_ptr<Value> make_copy() const override { return std::make_unique<Int64>(m_value); }

    void write(byte* buffer) const override { std::memcpy(buffer, &m_value, sizeof(m_value)); }
    void read(const byte* buffer) override { std::memcpy(&m_value, buffer, sizeof(m_value)); }
    TypeInfo type_info() const override { return TypeInfo{Type::Int64}; }

    void apply(value::Visitor& visitor) const override { visitor.visit(*this); }

    int64_t value() const { return m_value; }

private:
    int64_t m_value;
};


class Float32: public Value {
public:
    Float32() : m_value(0.0f) {}
    explicit Float32(float v) : m_value(v) {}
    std::unique_ptr<Value> make_copy() const override { return std::make_unique<Float32>(m_value); }

    void write(byte* buffer) const override { std::memcpy(buffer, &m_value, sizeof(m_value)); }
    void read(const byte* buffer) override { std::memcpy(&m_value, buffer, sizeof(m_value)); }
    TypeInfo type_info() const override { return TypeInfo{Type::Float32}; }

    void apply(value::Visitor& visitor) const override { visitor.visit(*this); }

    float value() const { return m_value; }

private:
    float m_value;
};


class Float64: public Value {
public:
    Float64() : m_value(0.0) {}
    explicit Float64(double v) : m_value(v) {}
    std::unique_ptr<Value> make_copy() const override { return std::make_unique<Float64>(m_value); }

    void write(byte* buffer) const override { std::memcpy(buffer, &m_value, sizeof(m_value)); }
    void read(const byte* buffer) override { std::memcpy(&m_value, buffer, sizeof(m_value)); }
    TypeInfo type_info() const override { return TypeInfo{Type::Float64}; }

    void apply(value::Visitor& visitor) const override { visitor.visit(*this); }

    double value() const { return m_value; }

private:
    double m_value;
};


// ------------- //
// Complex types //
// ------------- //

//        bool* m_bools;
//        uint8_t* m_bytes;
//        int32_t* m_int32s;
//        int64_t* m_int64s;
//        float* m_float32s;
//        double* m_float64s;


class String: public Value {
public:
    String() = default;
    explicit String(const std::string& v);
    explicit String(const char* cstr, size_t size);
    explicit String(size_t size, const HeapSlot& slot) : m_size(size), m_value(slot) {}
    String(const String& other) = default;
    String(String&& other) noexcept : m_size(other.m_size), m_value(other.m_value) {
        other.m_size = 0;
        other.m_value = HeapSlot{};
    }
    String& operator =(const String& rhs) = default;
    String& operator =(String&& rhs) noexcept;
    std::unique_ptr<Value> make_copy() const override { return std::make_unique<String>(m_size, m_value); }

    void write(byte* buffer) const override;
    void read(const byte* buffer) override;
    void incref() const override { m_value.incref(); }
    void decref() const override { m_value.decref(); }

    TypeInfo type_info() const override { return TypeInfo{Type::String}; }

    void apply(value::Visitor& visitor) const override { visitor.visit(*this); }

    std::string value() const { return {reinterpret_cast<const char*>(m_value.data()), m_size}; }

private:
    // UTF-8 encoded (size here is in bytes, not Chars!)
    size_t m_size = 0;
    HeapSlot m_value;
};


class List: public Value {
public:
    List() = default;
    explicit List(const Values& values);  // all values must have same type!
    explicit List(TypeInfo elem_type) : m_elem_type(std::move(elem_type)) {}
    explicit List(TypeInfo elem_type, size_t length, HeapSlot slot)
            : m_elem_type(std::move(elem_type)), m_length(length), m_elements(std::move(slot)) {}

    List(const List& other) = default;
    List(List&& other) noexcept = default;

    std::unique_ptr<Value> make_copy() const override { return std::make_unique<List>(m_elem_type, m_length, m_elements); }

    void write(byte* buffer) const override;
    void read(const byte* buffer) override;
    void incref() const override { m_elements.incref(); }
    void decref() const override { m_elements.decref(); }

    TypeInfo type_info() const override { return TypeInfo{Type::List, m_elem_type}; }

    void apply(value::Visitor& visitor) const override { visitor.visit(*this); }

    size_t length() const { return m_length; }
    std::unique_ptr<Value> get(size_t idx) const;

private:
    TypeInfo m_elem_type;
    size_t m_length = 0;  // number of elements
    HeapSlot m_elements;
};


struct Int32List: public List {
public:
    Int32List() : List(TypeInfo{Type::Int32}) {}
};


// Frozen vector of values - cannot add/modify items
class Tuple: public Value {
public:
    Tuple() = default;
    explicit Tuple(Values values) : m_values(std::move(values)) {}
    explicit Tuple(const TypeInfo& type_info);
    Tuple(const Tuple& other) : Tuple(other.values()) {}

    std::unique_ptr<Value> make_copy() const override { return std::make_unique<Tuple>(m_values); }

    void write(byte* buffer) const override;
    void read(const byte* buffer) override;
    void incref() const override;
    void decref() const override;

    TypeInfo type_info() const override;

    void apply(value::Visitor& visitor) const override { visitor.visit(*this); }

    const Values& values() const { return m_values; }

private:
    Values m_values;
};


class Lambda: public Value {
public:
    Lambda() : m_function(nullptr) {}
    explicit Lambda(Function& v);
    explicit Lambda(Function& v, Values&& nonlocals);
    explicit Lambda(Function& v, const HeapSlot& slot) : m_function(&v), m_closure(slot) {}

    std::unique_ptr<Value> make_copy() const override;

    void write(byte* buffer) const override;
    void read(const byte* buffer) override;
    void incref() const override { m_closure.incref(); }
    void decref() const override { m_closure.decref(); }

    TypeInfo type_info() const override;

    Function& function() { return *m_function; }
    const Function& function() const { return *m_function; }
    Tuple closure() const;

    void apply(value::Visitor& visitor) const override { visitor.visit(*this); }

private:
    Function* m_function;
    HeapSlot m_closure;
};


class Module: public Value {
public:
    Module() : m_module(nullptr) {}
    explicit Module(script::Module& v) : m_module(&v) {}
    std::unique_ptr<Value> make_copy() const override { return std::make_unique<Module>(*m_module); }

    // TODO
    void write(byte* buffer) const override {}
    void read(const byte* buffer) override {}

    TypeInfo type_info() const override { return TypeInfo{Type::Module}; }

    void apply(value::Visitor& visitor) const override { visitor.visit(*this); }

    const script::Module& module() const { return *m_module; }

private:
    script::Module* m_module;
};


} // namespace value


std::ostream& operator<<(std::ostream& os, const Value& o);


} // namespace xci::script

#endif // include guard
