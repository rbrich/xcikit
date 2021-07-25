// Value.h created on 2019-05-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_VALUE_H
#define XCI_SCRIPT_VALUE_H

#include "TypeInfo.h"
#include "Heap.h"
#include "Stream.h"
#include "Code.h"

#include <ostream>
#include <utility>
#include <vector>
#include <map>
#include <string_view>
#include <span>
#include <variant>
#include <cstring>
#include <cstdint>
#include <cstddef>  // byte

namespace xci::script {

using std::byte;
using std::move;

class Value;
struct ListV;
struct TupleV;
struct ClosureV;
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
class Bytes;
class String;
class List;
class Tuple;
class Closure;
class Module;

class Visitor {
public:
    virtual void visit(void) = 0;
    virtual void visit(bool) = 0;
    virtual void visit(byte) = 0;
    virtual void visit(char32_t) = 0;
    virtual void visit(uint32_t) = 0;
    virtual void visit(uint64_t) = 0;
    virtual void visit(int32_t) = 0;
    virtual void visit(int64_t) = 0;
    virtual void visit(float) = 0;
    virtual void visit(double) = 0;
    virtual void visit(std::string_view&&) = 0;
    virtual void visit(const ListV&) = 0;
    virtual void visit(const TupleV&) = 0;
    virtual void visit(const ClosureV&) = 0;
    virtual void visit(const script::Module*) = 0;
    virtual void visit(const script::Stream&) = 0;
};

class PartialVisitor : public Visitor {
    void visit(void) override {}
    void visit(bool) override {}
    void visit(byte) override {}
    void visit(char32_t) override {}
    void visit(uint32_t) override {}
    void visit(uint64_t) override {}
    void visit(int32_t) override {}
    void visit(int64_t) override {}
    void visit(float) override {}
    void visit(double) override {}
    void visit(std::string_view&&) override {}
    void visit(const ListV&) override {}
    void visit(const TupleV&) override {}
    void visit(const ClosureV&) override {}
    void visit(const script::Module*) override {}
    void visit(const script::Stream&) override {}
};

} // namespace value


struct StringV {
    StringV() = default;
    explicit StringV(std::string_view v);
    bool operator ==(const StringV& rhs) const { return value() == rhs.value(); }
    std::string_view value() const;

    HeapSlot slot;
};


struct ListV {
    ListV() = default;
    explicit ListV(size_t length, const TypeInfo& elem_type, const std::byte* elem_data = nullptr);
    explicit ListV(HeapSlot&& slot) : slot(move(slot)) {}
    bool operator ==(const ListV& rhs) const { return slot.slot() == rhs.slot.slot(); }  // same slot - cannot compare content without elem_type
    size_t length() const;
    Value value_at(size_t idx, const TypeInfo& elem_type) const;

    HeapSlot slot;
};


struct TupleV {
    TupleV(const TupleV& other);
    TupleV& operator =(const TupleV& other);

    explicit TupleV(Values&& vs);
    explicit TupleV(const TypeInfo::Subtypes& subtypes);
    bool operator ==(const TupleV& rhs) const { return values == rhs.values; }

    bool empty() const;
    size_t length() const;
    const Value& value_at(size_t idx) const { return values[idx]; }
    void foreach(const std::function<void(const Value&)>& cb) const;

    std::unique_ptr<Value[]> values;  // Void-terminated
};


struct ClosureV {
    explicit ClosureV() = default;
    explicit ClosureV(const Function& fn);
    explicit ClosureV(const Function& fn, Values&& values);
    bool operator ==(const ClosureV& rhs) const { return slot.slot() == rhs.slot.slot(); }  // same slot - cannot compare content without elem_type

    Function* function() const;
    value::Tuple closure() const;

    HeapSlot slot;
};


struct StreamV {
    StreamV() = default;
    explicit StreamV(const Stream& v);

    bool operator ==(const StreamV& rhs) const { return value() == rhs.value(); }
    Stream value() const;

    HeapSlot slot;
};


class Value {
public:
    struct StringTag {};
    struct ListTag {};
    struct ClosureTag {};
    struct StreamTag {};
    struct ModuleTag {};

    Value() = default;  // Void
    explicit Value(bool v) : m_value(v) {}  // Bool
    explicit Value(byte v) : m_value(v) {}  // Byte
    explicit Value(uint8_t v) : m_value(byte(v)) {}  // Byte
    explicit Value(char32_t v) : m_value(v) {}  // Char
    explicit Value(uint32_t v) : m_value(v) {}  // UInt32
    explicit Value(uint64_t v) : m_value(v) {}  // UInt64
    explicit Value(int32_t v) : m_value(v) {}  // Int32
    explicit Value(int64_t v) : m_value(v) {}  // Int64
    explicit Value(float v) : m_value(v) {}  // Float32
    explicit Value(double v) : m_value(v) {}  // Float64
    explicit Value(StringTag) : m_value(StringV{}) {}  // String
    explicit Value(std::string_view v) : m_value(StringV{v}) {}  // String
    explicit Value(ListTag) : m_value(ListV{}) {}  // List
    explicit Value(size_t length, const TypeInfo& elem_type) : m_value(ListV{length, elem_type}) {}  // List
    explicit Value(ListTag, HeapSlot&& slot) : m_value(ListV{move(slot)}) {}  // List
    explicit Value(ListV&& list_v) : m_value(move(list_v)) {}  // List
    explicit Value(const TypeInfo::Subtypes& subtypes) : m_value(TupleV{subtypes}) {}  // Tuple
    explicit Value(Values&& values) : m_value(TupleV{move(values)}) {}  // Tuple
    explicit Value(ClosureTag) : m_value(ClosureV{}) {}  // Closure
    explicit Value(const Function& fn) : m_value(ClosureV{fn}) {}  // Closure
    explicit Value(const Function& fn, Values&& values) : m_value(ClosureV{fn, move(values)}) {}  // Closure
    explicit Value(StreamTag) : m_value(StreamV{}) {}  // Stream
    explicit Value(const script::Stream& v) : m_value(StreamV{v}) {}  // Stream
    explicit Value(ModuleTag) : m_value((script::Module*) nullptr) {}  // Module
    explicit Value(script::Module& v) : m_value(&v) {}  // Module

    bool operator ==(const Value& rhs) const;

    // Serialize/deserialize the value
    // Note that if the value lives on Heap, this also affects refcount
    // To read value and also keep it in original location,
    // you MUST also call incref.
    // Returns number of bytes written/read
    size_t write(byte* buffer) const;
    size_t read(const byte* buffer);
    size_t size_on_stack() const;

    // Set when the value lives on heap
    const HeapSlot* heapslot() const;

    // Reference count management
    void incref() const;
    void decref() const;

    void apply(value::Visitor& visitor) const;

    Type type() const;
    bool is_void() const { return type() == Type::Void; }
    bool is_bool() const { return type() == Type::Bool; }
    bool is_callable() const { return type() == Type::Function; }

    // Load value from `other` and return true if the value is compatible
    // (can be statically casted). Return false otherwise.
    bool cast_from(const Value& src);

    bool negate();  // unary minus op
    bool modulus(const Value& rhs);

    template <class TBinFun>
    Value binary_op(const Value& rhs) {
        return std::visit([](const auto& l, const auto& r) -> Value {
            using TLhs = std::decay_t<decltype(l)>;
            using TRhs = std::decay_t<decltype(r)>;

            if constexpr (std::is_same_v<TLhs, TRhs> &&
                    (std::is_integral_v<TLhs> || std::is_floating_point_v<TLhs>))
                return Value( TBinFun{}(l, r) );

            if constexpr (std::is_same_v<TLhs, TRhs> && std::is_same_v<TLhs, byte>)
                return Value( TBinFun{}(uint8_t(l), uint8_t(r)) );

            return {};
        }, m_value, rhs.m_value);
    }

    // Cast to subtype, e.g.: `v.get<bool>()`
    template <class T> T& get() { return std::get<T>(m_value); }
    template <class T> const T& get() const { return std::get<T>(m_value); }

    int64_t to_int64() const;

    std::string_view get_string() const { return get<StringV>().value(); }
    void tuple_foreach(const std::function<void(const Value&)>& cb) const { return get<TupleV>().foreach(cb); }
    const script::Module& get_module() const { return *get<script::Module*>(); }

protected:
    using ValueVariant = std::variant<
            std::monostate,
            bool, byte, char32_t, uint32_t, uint64_t, int32_t, int64_t, float, double,
            StringV, ListV, TupleV, ClosureV, StreamV,
            script::Module*
        >;
    ValueVariant m_value;
};


Value create_value(const TypeInfo& type_info);
Value create_value(Type type);


template<typename T>
concept ValueWithTypeInfo = requires(const T& v) {
    std::is_base_of_v<Value, T>;
    v.type_info();
};


template<typename T>
concept ValueT = requires(const T& v) {
    std::is_base_of_v<Value, T>;
};


class Values {
public:
    Values() = default;
    explicit Values(std::initializer_list<Value> values) : m_items(values) {}

    // reserve number of values
    void reserve(size_t n) { m_items.reserve(n); }
    // get number of values
    size_t size() const { return m_items.size(); }
    bool empty() const { return m_items.empty(); }

    // get size of all values in bytes
    size_t size_on_stack() const;

    void add(Value&& value) { m_items.emplace_back(std::forward<Value>(value)); }

    const Value& operator[](size_t i) const { return m_items[i]; }

    bool operator==(const Values& rhs) const { return m_items == rhs.m_items; }
    bool operator!=(const Values& rhs) const { return m_items != rhs.m_items; }

    using iterator = std::vector<Value>::iterator;
    using const_iterator = std::vector<Value>::const_iterator;
    iterator begin() { return m_items.begin(); }
    iterator end() { return m_items.end(); }
    const_iterator begin() const { return m_items.begin(); }
    const_iterator end() const { return m_items.end(); }
    const Value& back() const { return m_items.back(); }

private:
    std::vector<Value> m_items;
};


class TypedValue {
public:
    TypedValue() = default;

    template <ValueWithTypeInfo T> explicit TypedValue(const T& v) : m_value(v), m_type_info(v.type_info()) {}

    TypedValue(TypeInfo type_info) : m_value(create_value(type_info)), m_type_info(move(type_info)) {}
    TypedValue(Value value, TypeInfo type_info);

    bool operator ==(const TypedValue& rhs) const { return m_value == rhs.m_value; }

    const Value& value() const { return m_value; }
    const TypeInfo& type_info() const { return m_type_info; }
    Type type() const { return m_type_info.type(); }

    void incref() const { m_value.incref(); }
    void decref() const { m_value.decref(); }

    void apply(value::Visitor& visitor) const { m_value.apply(visitor); }

    bool is_void() const { return type() == Type::Void; }
    bool is_bool() const { return type() == Type::Bool; }

    template <class T> const T& get() const { return m_value.get<T>(); }

private:
    Value m_value;
    TypeInfo m_type_info;
};


class TypedValues {
public:
    // reserve number of values
    void reserve(size_t n) { m_items.reserve(n); }
    // get number of values
    size_t size() const { return m_items.size(); }
    bool empty() const { return m_items.empty(); }

    void add(TypedValue&& value) { m_items.emplace_back(std::forward<TypedValue>(value)); }

    const TypedValue& operator[](size_t i) const { return m_items[i]; }

    bool operator==(const TypedValues& rhs) const { return m_items == rhs.m_items; }
    bool operator!=(const TypedValues& rhs) const { return m_items != rhs.m_items; }

    using iterator = std::vector<TypedValue>::iterator;
    using const_iterator = std::vector<TypedValue>::const_iterator;
    iterator begin() { return m_items.begin(); }
    iterator end() { return m_items.end(); }
    const_iterator begin() const { return m_items.begin(); }
    const_iterator end() const { return m_items.end(); }
    const TypedValue& back() const { return m_items.back(); }

private:
    std::vector<TypedValue> m_items;
};


namespace value {

// ----------- //
// Plain types //
// ----------- //


// Empty value, doesn't carry any information
class Void: public Value {
public:
    void value() const {}
    TypeInfo type_info() const { return TypeInfo{Type::Void}; }
};


class Bool: public Value {
public:
    Bool() : Value(false) {}
    explicit Bool(bool v) : Value(v) {}
    TypeInfo type_info() const { return TypeInfo{Type::Bool}; }
    bool value() const { return std::get<bool>(m_value); }
    void set_value(bool v) { m_value = v; }
};


class Byte: public Value {
public:
    Byte() : Value(byte(0)) {}
    explicit Byte(byte v) : Value(v) {}
    explicit Byte(uint8_t v) : Value(byte(v)) {}
    explicit Byte(std::string_view utf8);
    TypeInfo type_info() const { return TypeInfo{Type::Byte}; }
    uint8_t value() const { return (uint8_t) std::get<byte>(m_value); }
    void set_value(byte v) { m_value = v; }
    void set_value(uint8_t v) { m_value = byte(v); }
};


class Char: public Value {
public:
    Char() : Value(char32_t(0)) {}
    explicit Char(char32_t v) : Value(v) {}
    explicit Char(std::string_view utf8);
    TypeInfo type_info() const { return TypeInfo{Type::Char}; }
    char32_t value() const { return std::get<char32_t>(m_value); }
    void set_value(char32_t v) { m_value = v; }
};


class UInt32: public Value {
public:
    UInt32() : Value(uint32_t(0)) {}
    explicit UInt32(uint32_t v) : Value(v) {}
    TypeInfo type_info() const { return ti_uint32(); }
    uint32_t value() const { return std::get<uint32_t>(m_value); }
    void set_value(uint32_t v) { m_value = v; }
};


class UInt64: public Value {
public:
    UInt64() : Value(uint64_t(0)) {}
    explicit UInt64(uint64_t v) : Value(v) {}
    TypeInfo type_info() const { return ti_uint64(); }
    uint64_t value() const { return std::get<uint64_t>(m_value); }
    void set_value(uint64_t v) { m_value = v; }
};


class Int32: public Value {
public:
    Int32() : Value(int32_t(0)) {}
    explicit Int32(int32_t v) : Value(v) {}
    TypeInfo type_info() const { return ti_int32(); }
    int32_t value() const { return std::get<int32_t>(m_value); }
    void set_value(int32_t v) { m_value = v; }
};


class Int64: public Value {
public:
    Int64() : Value(int64_t(0)) {}
    explicit Int64(int64_t v) : Value(v) {}
    TypeInfo type_info() const { return ti_int64(); }
    int64_t value() const { return std::get<int64_t>(m_value); }
    void set_value(int64_t v) { m_value = v; }
};


class Float32: public Value {
public:
    Float32() : Value(0.0f) {}
    explicit Float32(float v) : Value(v) {}
    TypeInfo type_info() const { return TypeInfo{Type::Float32}; }
    float value() const { return std::get<float>(m_value); }
    void set_value(float v) { m_value = v; }
};


class Float64: public Value {
public:
    Float64() : Value(0.0) {}
    explicit Float64(double v) : Value(v) {}
    TypeInfo type_info() const { return TypeInfo{Type::Float64}; }
    double value() const { return std::get<double>(m_value); }
    void set_value(double v) { m_value = v; }
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
    String() : Value(Value::StringTag{}) {}
    explicit String(std::string_view v) : Value(v) {}
    TypeInfo type_info() const { return TypeInfo{Type::String}; }
    std::string_view value() const { return get_string(); }
    void set_value(std::string_view v) { m_value = StringV(v); }
};


class List: public Value {
public:
    List() : Value(Value::ListTag{}) {}
    List(size_t length, const TypeInfo& elem_type) : Value(length, elem_type) {}
    explicit List(HeapSlot&& slot) : Value(Value::ListTag{}, move(slot)) {}

    size_t length() const { return get<ListV>().length(); }
    Value value_at(size_t idx, const TypeInfo& elem_type) const { return get<ListV>().value_at(idx, elem_type); }
    TypedValue typed_value_at(size_t idx, const TypeInfo& elem_type) const { return {value_at(idx, elem_type), elem_type}; }
};


// [Bytes] has some special handling, e.g. it's dumped as b"abc", not [1,2,3]
class Bytes: public List {
public:
    Bytes() = default;
    explicit Bytes(std::span<const byte> v);

    TypeInfo type_info() const { return ti_bytes(); }

    std::span<const byte> value() const { return {heapslot()->data() + 6, length()}; }
};


class Int32List: public List {
public:
    TypeInfo type_info() const { return ti_list(ti_int32()); }
};


// Frozen vector of values - cannot add/modify items
class Tuple: public Value {
public:
    Tuple() : Value(Values{}) {}
    Tuple(std::initializer_list<Value> values) : Value(Values{values}) {}
    explicit Tuple(Values&& values) : Value(move(values)) {}
    explicit Tuple(const TypeInfo::Subtypes& subtypes) : Value(subtypes) {}

    bool empty() const { return get<TupleV>().empty(); }
    size_t length() const { return get<TupleV>().length(); }
    const Value& value_at(size_t idx) const { return get<TupleV>().value_at(idx); }
};


class Closure: public Value {
public:
    Closure() : Value(Value::ClosureTag{}) {}
    explicit Closure(const Function& fn) : Value(fn) {}
    explicit Closure(const Function& fn, Values&& values) : Value(fn, move(values)) {}
    TypeInfo type_info() const { return TypeInfo{Type::Function}; }

    Function* function() const { return get<ClosureV>().function(); }
    value::Tuple closure() const { return get<ClosureV>().closure(); }
};


class Stream: public Value {
public:
    Stream() : Value(Value::StreamTag{}) {}
    explicit Stream(const script::Stream& v) : Value(v) {}
    TypeInfo type_info() const { return TypeInfo{Type::Stream}; }

    script::Stream value() const { return get<StreamV>().value(); }
};


class Module: public Value {
public:
    Module() : Value(Value::ModuleTag{}) {}
    explicit Module(script::Module& v) : Value(v) {}

    script::Module& value() { return *get<script::Module*>(); }
    const script::Module& value() const { return *get<script::Module*>(); }
};

} // namespace value


std::ostream& operator<<(std::ostream& os, const TypedValue& o);
std::ostream& operator<<(std::ostream& os, const Value& o);


} // namespace xci::script

#endif // include guard
