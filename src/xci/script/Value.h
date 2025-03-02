// Value.h created on 2019-05-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2025 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_VALUE_H
#define XCI_SCRIPT_VALUE_H

#include "TypeInfo.h"
#include "Heap.h"
#include "Stream.h"
#include "Code.h"

#include <xci/compat/int128.h>
#include <xci/compat/float128.h>

#include <ostream>
#include <utility>
#include <vector>
#include <map>
#include <string_view>
#include <span>
#include <variant>
#include <cstring>
#include <cstdint>

namespace xci::script {

class Value;
struct ListV;
struct TupleV;
struct ClosureV;
struct TypeIndexV;
class Values;
class Function;
class Module;


namespace value {

class Tuple;

class Visitor {
public:
    virtual void visit(bool) = 0;
    virtual void visit(char32_t) = 0;
    virtual void visit(uint8_t) = 0;
    virtual void visit(uint16_t) = 0;
    virtual void visit(uint32_t) = 0;
    virtual void visit(uint64_t) = 0;
    virtual void visit(uint128) = 0;
    virtual void visit(int8_t) = 0;
    virtual void visit(int16_t) = 0;
    virtual void visit(int32_t) = 0;
    virtual void visit(int64_t) = 0;
    virtual void visit(int128) = 0;
    virtual void visit(float) = 0;
    virtual void visit(double) = 0;
    virtual void visit(float128) = 0;
    virtual void visit(std::string_view&&) = 0;
    virtual void visit(const ListV&) = 0;
    virtual void visit(const TupleV&) = 0;
    virtual void visit(const ClosureV&) = 0;
    virtual void visit(const script::Module*) = 0;
    virtual void visit(const script::Stream&) = 0;
    virtual void visit(const TypeIndexV&) = 0;
};

class PartialVisitor : public Visitor {
    void visit(bool) override {}
    void visit(char32_t) override {}
    void visit(uint8_t) override {}
    void visit(uint16_t) override {}
    void visit(uint32_t) override {}
    void visit(uint64_t) override {}
    void visit(uint128) override {}
    void visit(int8_t) override {}
    void visit(int16_t) override {}
    void visit(int32_t) override {}
    void visit(int64_t) override {}
    void visit(int128) override {}
    void visit(float) override {}
    void visit(double) override {}
    void visit(float128) override {}
    void visit(std::string_view&&) override {}
    void visit(const ListV&) override {}
    void visit(const TupleV&) override {}
    void visit(const ClosureV&) override {}
    void visit(const script::Module*) override {}
    void visit(const script::Stream&) override {}
    void visit(const TypeIndexV&) override {}
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
    explicit ListV(size_t length, const TypeInfo& elem_type, const void* elem_data = nullptr);
    explicit ListV(HeapSlot&& slot) : slot(std::move(slot)) {}
    bool operator ==(const ListV& rhs) const { return slot.slot() == rhs.slot.slot(); }  // same slot - cannot compare content without elem_type
    size_t length() const;
    const std::byte* raw_data() const;
    std::byte* raw_data() { return (std::byte*) const_cast<const ListV*>(this)->raw_data(); }
    Value value_at(size_t idx, const TypeInfo& elem_type) const;
    void set_value(size_t idx, const Value& v);

    /// Slice the list. Indexes work similarly to Python.
    /// Automatically copies the list on heap when it has more than 1 reference.
    /// Works in-place otherwise.
    void slice(int64_t begin, int64_t end, int64_t step, const TypeInfo& elem_type);

    /// Extend this list by concatenating another list (of same type).
    /// Automatically copies the list on heap when it has more than 1 reference.
    /// Works in-place otherwise.
    void extend(const ListV& rhs, const TypeInfo& elem_type);

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
    const Value& value_at(size_t idx) const;
    void foreach(const std::function<void(const Value&)>& cb) const;

    std::unique_ptr<Value[]> values;  // Unknown-terminated
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


struct ModuleV {
    ModuleV() = default;
    explicit ModuleV(script::Module& v) : module_ptr(&v) {}

    bool operator ==(const ModuleV& rhs) const { return value() == rhs.value(); }
    const script::Module* value() const { return module_ptr; }

    script::Module* module_ptr = nullptr;
};


struct TypeIndexV {
    TypeIndexV() = default;
    explicit TypeIndexV(Index v) : type_index(v) {}

    bool operator ==(const TypeIndexV& rhs) const { return value() == rhs.value(); }
    operator Index() const { return type_index; }
    Index value() const { return type_index; }

    Index type_index = no_index;
};


class Value {
public:
    struct StringTag {};
    struct ListTag {};
    struct ClosureTag {};
    struct StreamTag {};
    struct ModuleTag {};
    struct TypeIndexTag {};

    Value() = default;  // Unknown (invalid value)
    explicit Value(bool v) : m_value(v) {}  // Bool
    explicit Value(char32_t v) : m_value(v) {}  // Char
    explicit Value(uint8_t v) : m_value(v) {}  // UInt8
    explicit Value(uint16_t v) : m_value(v) {}  // UInt16
    explicit Value(uint32_t v) : m_value(v) {}  // UInt32
    explicit Value(uint64_t v) : m_value(v) {}  // UInt64
    explicit Value(uint128 v) : m_value(v) {}  // UInt128
    explicit Value(int8_t v) : m_value(v) {}  // Int8
    explicit Value(int16_t v) : m_value(v) {}  // Int16
    explicit Value(int32_t v) : m_value(v) {}  // Int32
    explicit Value(int64_t v) : m_value(v) {}  // Int64
    explicit Value(int128 v) : m_value(v) {}  // Int128
    explicit Value(float v) : m_value(v) {}  // Float32
    explicit Value(double v) : m_value(v) {}  // Float64
    explicit Value(float128 v) : m_value(v) {}  // Float128
    explicit Value(StringTag) : m_value(StringV{}) {}  // String
    explicit Value(std::string_view v) : m_value(StringV{v}) {}  // String
    explicit Value(ListTag) : m_value(ListV{}) {}  // List
    explicit Value(size_t length, const TypeInfo& elem_type) : m_value(ListV{length, elem_type}) {}  // List
    explicit Value(ListTag, HeapSlot&& slot) : m_value(ListV{std::move(slot)}) {}  // List
    explicit Value(ListV&& list_v) : m_value(std::move(list_v)) {}  // List
    explicit Value(const TypeInfo::Subtypes& subtypes) : m_value(TupleV{subtypes}) {}  // Tuple
    explicit Value(Values&& values) : m_value(TupleV{std::move(values)}) {}  // Tuple
    explicit Value(ClosureTag) : m_value(ClosureV{}) {}  // Closure
    explicit Value(const Function& fn) : m_value(ClosureV{fn}) {}  // Closure
    explicit Value(const Function& fn, Values&& values) : m_value(ClosureV{fn, std::move(values)}) {}  // Closure
    explicit Value(StreamTag) : m_value(StreamV{}) {}  // Stream
    explicit Value(const script::Stream& v) : m_value(StreamV{v}) {}  // Stream
    explicit Value(ModuleTag) : m_value(ModuleV{}) {}  // Module
    explicit Value(script::Module& v) : m_value(ModuleV{v}) {}  // Module
    explicit Value(TypeIndexTag) : m_value(TypeIndexV{}) {}  // TypeIndex
    explicit Value(TypeIndexTag, Index v) : m_value(TypeIndexV{v}) {}  // TypeIndex

    bool operator ==(const Value& rhs) const;

    // Serialize/deserialize the value
    // Note that if the value lives on Heap, this also affects refcount
    // To read value and also keep it in original location,
    // you MUST also call incref.
    // Returns number of bytes written/read
    size_t write(std::byte* buffer) const;
    size_t read(const std::byte* buffer);
    size_t size_on_stack() const noexcept;

    // Set when the value lives on heap
    const HeapSlot* heapslot() const;

    // Reference count management
    void incref() const;
    void decref();

    void apply(value::Visitor& visitor) const;

    Type type() const;
    bool is_unknown() const { return type() == Type::Unknown; }
    bool is_void() const { return type() == Type::Tuple && get<TupleV>().empty(); }
    bool is_bool() const { return type() == Type::Bool; }
    bool is_function() const { return type() == Type::Function; }

    // Load value from `other` and return true if the value is compatible
    // (can be statically cast). Return false otherwise.
    bool cast_from(const Value& src);

    bool negate();  // unary minus op

    template <class F>
    Value binary_op(const Value& rhs, F&& f) {
        return std::visit([&rhs, &f](const auto& l) -> Value {
            using T = std::decay_t<decltype(l)>;
            if constexpr ((std::is_floating_point_v<T> ||
                           std::is_integral_v<T> ||
                           std::is_same_v<T, uint128> || std::is_same_v<T, int128>
                          ) && !std::is_same_v<T, bool> && !std::is_same_v<T, char32_t>)
            {
                if (std::holds_alternative<T>(rhs.m_value))
                    return f(l, rhs.get<T>());
            }
            XCI_UNREACHABLE;
        }, m_value);
    }

    // Cast to subtype, e.g.: `v.get<bool>()`
    template <class T> T& get() { return std::get<T>(m_value); }
    template <class T> const T& get() const { return std::get<T>(m_value); }

    int64_t to_int64() const;

    std::string_view get_string() const { return get<StringV>().value(); }
    void tuple_foreach(const std::function<void(const Value&)>& cb) const { return get<TupleV>().foreach(cb); }
    const script::Module& get_module() const { return *get<ModuleV>().module_ptr; }

    // -------------------------------------------------------------------------
    // Serialization

    template <class Archive>
    void serialize(Archive& ar) {
        std::visit([&ar](auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                // Unknown
            } else if constexpr (std::is_trivial_v<T>) {
                ar(v);
            } else {
                // String etc.
                // TODO
            }
        }, m_value);
    }

protected:
    using ValueVariant = std::variant<
            std::monostate,  // Unknown (invalid value)
            bool, char32_t,
            uint8_t, uint16_t, uint32_t, uint64_t, uint128,
            int8_t, int16_t, int32_t, int64_t, int128,
            float, double, float128,
            StringV, ListV, TupleV, ClosureV, StreamV, ModuleV, TypeIndexV
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
    using value_type = Value;
    using reference = Value&;
    using const_reference = const Value&;

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

    const_reference operator[](size_t i) const { return m_items[i]; }

    bool operator==(const Values& rhs) const { return m_items == rhs.m_items; }

    using iterator = std::vector<Value>::iterator;
    using const_iterator = std::vector<Value>::const_iterator;
    iterator begin() { return m_items.begin(); }
    iterator end() { return m_items.end(); }
    const_iterator begin() const { return m_items.begin(); }
    const_iterator end() const { return m_items.end(); }
    const_reference back() const { return m_items.back(); }

private:
    std::vector<Value> m_items;
};


class TypedValue {
public:
    TypedValue() = default;

    template <ValueWithTypeInfo T> explicit TypedValue(const T& v) : m_value(v), m_type_info(v.type_info()) {}

    explicit TypedValue(TypeInfo type_info) : m_value(create_value(type_info)), m_type_info(std::move(type_info)) {}
    TypedValue(Value value, TypeInfo type_info);

    bool operator ==(const TypedValue& rhs) const { return m_value == rhs.m_value; }

    const Value& value() const { return m_value; }
    const TypeInfo& type_info() const { return m_type_info; }
    Type type() const { return m_type_info.type(); }

    void incref() const { m_value.incref(); }
    void decref() { m_value.decref(); }

    void apply(value::Visitor& visitor) const { m_value.apply(visitor); }

    bool is_unknown() const { return m_type_info.is_unknown(); }
    bool is_void() const { return m_type_info.is_void(); }
    bool is_bool() const { return m_type_info.is_bool(); }
    bool is_string() const { return m_type_info.is_string(); }

    template <class T> T& get() { return m_value.get<T>(); }
    template <class T> const T& get() const { return m_value.get<T>(); }

private:
    Value m_value;
    TypeInfo m_type_info;
};


class TypedValues {
public:
    using value_type = TypedValue;
    using reference = TypedValue&;
    using const_reference = const TypedValue&;

    // reserve number of values
    void reserve(size_t n) { m_items.reserve(n); }
    // get number of values
    size_t size() const { return m_items.size(); }
    bool empty() const { return m_items.empty(); }

    void add(TypedValue&& value) { m_items.emplace_back(std::forward<TypedValue>(value)); }
    reference emplace_back() { return m_items.emplace_back(); }

    reference operator[](size_t i) { return m_items[i]; }
    const_reference operator[](size_t i) const { return m_items[i]; }

    bool operator==(const TypedValues& rhs) const { return m_items == rhs.m_items; }

    using iterator = std::vector<TypedValue>::iterator;
    using const_iterator = std::vector<TypedValue>::const_iterator;
    iterator begin() { return m_items.begin(); }
    iterator end() { return m_items.end(); }
    const_iterator begin() const { return m_items.begin(); }
    const_iterator end() const { return m_items.end(); }
    const_reference back() const { return m_items.back(); }
    reference back() { return m_items.back(); }

private:
    std::vector<TypedValue> m_items;
};


namespace value {

// ----------- //
// Plain types //
// ----------- //


class Bool: public Value {
public:
    Bool() : Value(false) {}
    explicit Bool(bool v) : Value(v) {}
    TypeInfo type_info() const { return TypeInfo{Type::Bool}; }
    bool value() const { return std::get<bool>(m_value); }
    void set_value(bool v) { m_value = v; }
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



class UInt8: public Value {
public:
    UInt8() : Value(uint8_t(0)) {}
    explicit UInt8(std::byte v) : Value(uint8_t(v)) {}
    explicit UInt8(uint8_t v) : Value(v) {}
    explicit UInt8(std::string_view str);
    TypeInfo type_info() const { return TypeInfo{Type::UInt8}; }
    uint8_t value() const { return std::get<uint8_t>(m_value); }
    void set_value(std::byte v) { m_value = uint8_t(v); }
    void set_value(uint8_t v) { m_value = v; }
};


class UInt16: public Value {
public:
    UInt16() : Value(uint16_t(0)) {}
    explicit UInt16(uint16_t v) : Value(v) {}
    TypeInfo type_info() const { return ti_uint16(); }
    uint16_t value() const { return std::get<uint16_t>(m_value); }
    void set_value(uint16_t v) { m_value = v; }
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


class UInt128: public Value {
public:
    UInt128() : Value(uint128(0)) {}
    explicit UInt128(uint128 v) : Value(v) {}
    TypeInfo type_info() const { return ti_uint128(); }
    uint128 value() const { return std::get<uint128>(m_value); }
    void set_value(uint128 v) { m_value = v; }
};


class Int8: public Value {
public:
    Int8() : Value(int8_t(0)) {}
    explicit Int8(int8_t v) : Value(v) {}
    TypeInfo type_info() const { return ti_int8(); }
    int8_t value() const { return std::get<int8_t>(m_value); }
    void set_value(int8_t v) { m_value = v; }
};


class Int16: public Value {
public:
    Int16() : Value(int16_t(0)) {}
    explicit Int16(int16_t v) : Value(v) {}
    TypeInfo type_info() const { return ti_int16(); }
    int16_t value() const { return std::get<int16_t>(m_value); }
    void set_value(int16_t v) { m_value = v; }
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


class Int128: public Value {
public:
    Int128() : Value(int128(0)) {}
    explicit Int128(int128 v) : Value(v) {}
    TypeInfo type_info() const { return ti_int128(); }
    int128 value() const { return std::get<int128>(m_value); }
    void set_value(int128 v) { m_value = v; }
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

class Float128: public Value {
public:
    Float128() : Value((float128)(0.0)) {}
    explicit Float128(float128 v) : Value(v) {}
    TypeInfo type_info() const { return TypeInfo{Type::Float128}; }
    float128 value() const { return std::get<float128>(m_value); }
    void set_value(float128 v) { m_value = v; }
};


using Int = Int64;
using UInt = UInt64;
using Float = Float64;

// ------------- //
// Complex types //
// ------------- //


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
    explicit List(HeapSlot&& slot) : Value(Value::ListTag{}, std::move(slot)) {}

    size_t length() const { return get<ListV>().length(); }
    Value value_at(size_t idx, const TypeInfo& elem_type) const { return get<ListV>().value_at(idx, elem_type); }
    TypedValue typed_value_at(size_t idx, const TypeInfo& elem_type) const { return {value_at(idx, elem_type), elem_type}; }
    void set_value(size_t idx, const Value& v) { get<ListV>().set_value(idx, v); }
};


// [Bytes] has some special handling, e.g. it's dumped as b"abc", not [1,2,3]
class Bytes: public List {
public:
    Bytes() = default;
    explicit Bytes(std::span<const std::byte> v);

    TypeInfo type_info() const { return ti_bytes(); }

    std::span<const std::byte> value() const { return {heapslot()->data() + 6, length()}; }
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
    explicit Tuple(Values&& values) : Value(std::move(values)) {}
    explicit Tuple(const TypeInfo::Subtypes& subtypes) : Value(subtypes) {}

    bool empty() const { return get<TupleV>().empty(); }
    size_t length() const { return get<TupleV>().length(); }
    const Value& value_at(size_t idx) const { return get<TupleV>().value_at(idx); }
};


class Closure: public Value {
public:
    Closure() : Value(Value::ClosureTag{}) {}
    explicit Closure(const Function& fn) : Value(fn) {}
    explicit Closure(const Function& fn, Values&& values) : Value(fn, std::move(values)) {}
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

    script::Module& value() { return *get<ModuleV>().module_ptr; }
    const script::Module& value() const { return get_module(); }
};


class TypeIndex: public Value {
public:
    TypeIndex() : Value(Value::TypeIndexTag{}) {}
    explicit TypeIndex(Index v) : Value(Value::TypeIndexTag{}, v) {}
    TypeInfo type_info() const { return ti_type_index(); }
    Index value() const { return std::get<TypeIndexV>(m_value).value(); }
    void set_value(Index v) { m_value = TypeIndexV{v}; }
};


} // namespace value


template<class Archive>
void save(Archive& archive, const TypedValue& value)
{
    archive("type_info", value.type_info());
    archive("value", value.value());
}

template<class Archive>
void load(Archive& archive, TypedValue& value)
{
    TypeInfo type_info;
    archive(type_info);

    Value pure_value { create_value(type_info) };
    archive(pure_value);
    value = TypedValue(std::move(pure_value), std::move(type_info));
}


std::ostream& operator<<(std::ostream& os, const TypedValue& o);
std::ostream& operator<<(std::ostream& os, const Value& o);


} // namespace xci::script

template <> struct fmt::formatter<xci::script::TypedValue> : ostream_formatter {};
template <> struct fmt::formatter<xci::script::Value> : ostream_formatter {};

#endif // include guard
