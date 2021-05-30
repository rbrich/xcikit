// Value.cpp created on 2019-05-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Value.h"
#include "Function.h"
#include "Module.h"
#include "Error.h"
#include <xci/data/coding/leb128.h>
#include <xci/core/string.h>
#include <xci/core/log.h>
#include <xci/core/template/helpers.h>
#include <range/v3/view/transform.hpp>
#include <range/v3/to_container.hpp>
#include <range/v3/numeric/accumulate.hpp>
#include <numeric>
#include <sstream>
#include <limits>
#include <cassert>

namespace xci::script {

using xci::data::leb128_length;
using xci::data::leb128_encode;
using xci::data::leb128_decode;
using namespace xci::core;
using ranges::cpp20::views::transform;
using ranges::to;
using ranges::accumulate;
using std::move;
using std::make_unique;


template<typename T>
concept HasHeapSlot = requires(const T& v) {
    v.slot;
    std::is_same_v<decltype(v.slot), HeapSlot>;
};


Value create_value(const TypeInfo& type_info)
{
    const auto type = type_info.type();
    switch (type) {
        default:
            return create_value(type);
        case Type::List:
            if (type_info.elem_type() == TypeInfo{Type::Byte})
                return value::Bytes();  // List subclass, with special output formatting
            else
                return value::List();
        case Type::Tuple:
            return value::Tuple{type_info.subtypes()};
        case Type::Struct:
            auto subtypes = type_info.struct_items()
                | transform([](const TypeInfo::StructItem& item) { return item.second; })
                | to<std::vector>();
            return value::Tuple{subtypes};
    }
    return {};
}


Value create_value(Type type)
{
    switch (type) {
        case Type::Unknown: assert(!"Cannot create Value of Unknown type"); break;
        case Type::Void: return value::Void{};
        case Type::Bool: return value::Bool{};
        case Type::Byte: return value::Byte{};
        case Type::Char: return value::Char{};
        case Type::Int32: return value::Int32{};
        case Type::Int64: return value::Int64{};
        case Type::Float32: return value::Float32{};
        case Type::Float64: return value::Float64{};
        case Type::String: return value::String{};
        case Type::List: return value::List{};
        case Type::Tuple: return value::Tuple{};
        case Type::Struct: return value::Tuple{};  // struct differs only in type, otherwise it's just a tuple
        case Type::Function: return value::Closure{};
        case Type::Stream: return value::Stream{};
        case Type::Module: return value::Module{};
        case Type::Named: assert(!"Cannot create empty Value of Named type"); break;
    }
    return {};
}


bool Value::operator==(const Value& rhs) const
{
    return std::visit([](const auto& a, const auto& b) -> bool {
        using TA = std::decay_t<decltype(a)>;
        using TB = std::decay_t<decltype(b)>;
        if constexpr (std::is_same_v<TA, TB>)
            return a == b;
        else
            return false;  // different types
    }, m_value, rhs.m_value);
}


size_t Value::write(byte* buffer) const
{
    static_assert(sizeof(bool) == 1);
    return std::visit([buffer](auto&& v) -> size_t {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>)
            return 0;  // Void
        else if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, byte>) {
            *buffer = byte(v);
            return 1;
        } else if constexpr (HasHeapSlot<T>) {
            const byte* slot_ptr = v.slot.slot();
            std::memcpy(buffer, &slot_ptr, sizeof(slot_ptr));
            return sizeof(byte*);
        } else if constexpr (std::is_same_v<T, TupleV>) {
            byte* ofs = buffer;
            for (Value* it = v.values.get(); !it->is_void(); ++it)
                ofs += it->write(ofs);
            return ofs - buffer;
        } else if constexpr(std::is_trivially_copyable_v<T>) {
            std::memcpy(buffer, &v, sizeof(T));
            return sizeof(T);
        }
    }, m_value);
}


size_t Value::read(const byte* buffer)
{
    return std::visit([buffer](auto& v) -> size_t {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>)
            return 0;  // Void
        else if constexpr (std::is_same_v<T, bool>) {
            v = bool(*buffer);
            return 1;
        } else if constexpr (std::is_same_v<T, byte>) {
            v = *buffer;
            return 1;
        } else if constexpr (HasHeapSlot<T>) {
            byte* slot_ptr;
            std::memcpy(&slot_ptr, buffer, sizeof(byte*));
            v.slot = HeapSlot{slot_ptr};
            return sizeof(byte*);
        } else if constexpr (std::is_same_v<T, TupleV>) {
            const byte* ofs = buffer;
            for (Value* it = v.values.get(); !it->is_void(); ++it)
                ofs += it->read(ofs);
            return ofs - buffer;
        } else if constexpr(std::is_trivially_copyable_v<T>) {
            std::memcpy(&v, buffer, sizeof(T));
            return sizeof(T);
        }
    }, m_value);
}


size_t Value::size_on_stack() const
{
    static_assert(sizeof(bool) == 1);
    return std::visit([](auto&& v) -> size_t {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>)
            return 0;  // Void
        else if constexpr (std::is_same_v<T, TupleV>) {
            size_t res = 0;
            for (Value* it = v.values.get(); !it->is_void(); ++it)
                res += it->size_on_stack();
            return res;
        } else
            return sizeof(T);
    }, m_value);
}


const HeapSlot* Value::heapslot() const
{
    return std::visit([](auto& v) -> const HeapSlot* {
        using T = std::decay_t<decltype(v)>;
        if constexpr (HasHeapSlot<T>)
            return &v.slot;
        else
            return nullptr;
    }, m_value);
}



void Value::incref() const
{
    return std::visit([](auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (HasHeapSlot<T>)
            v.slot.incref();
        else if constexpr (std::is_same_v<T, TupleV>) {
            for (Value* it = v.values.get(); !it->is_void(); ++it)
                it->incref();
        }
    }, m_value);
}


void Value::decref() const
{
    return std::visit([](auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (HasHeapSlot<T>)
            v.slot.decref();
        else if constexpr (std::is_same_v<T, TupleV>) {
            for (Value* it = v.values.get(); !it->is_void(); ++it)
                it->decref();
        }
    }, m_value);
}


void Value::apply(value::Visitor& visitor) const
{
    std::visit([&visitor](auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>)
            visitor.visit();  // Void
        else if constexpr (std::is_same_v<T, StringV> || std::is_same_v<T, StreamV>)
            visitor.visit(v.value());
        else
            visitor.visit(v);
    }, m_value);
}


Type Value::type() const
{
    return std::visit(overloaded {
            [](std::monostate) { return Type::Void; },
            [](bool) { return Type::Bool; },
            [](byte) { return Type::Byte; },
            [](char32_t) { return Type::Char; },
            [](int32_t) { return Type::Int32; },
            [](int64_t) { return Type::Int64; },
            [](float) { return Type::Float32; },
            [](double) { return Type::Float64; },
            [](const StringV&) { return Type::String; },
            [](const ListV&) { return Type::List; },
            [](const TupleV&) { return Type::Tuple; },
            [](const ClosureV&) { return Type::Function; },
            [](const StreamV&) { return Type::Stream; },
            [](const script::Module*) { return Type::Module; },
    }, m_value);
}


bool Value::cast_from(const Value& src)
{
    return std::visit([](auto& to, const auto& from) -> bool {
        using TTo = std::decay_t<decltype(to)>;
        using TFrom = std::decay_t<decltype(from)>;

        // same types or cast to Void -> noop
        if constexpr (std::is_same_v<TFrom, TTo> || std::is_same_v<TTo, std::monostate>)
            return true;

        // int/float -> int/float
        if constexpr
                ((std::is_integral_v<TFrom> || std::is_floating_point_v<TFrom> || std::is_same_v<TFrom, byte>)
                && (std::is_integral_v<TTo> || std::is_floating_point_v<TTo> || std::is_same_v<TTo, byte>))
        {
            to = static_cast<TTo>(from);
            return true;
        }

        // Complex types or cast from Void
        return false;
    }, m_value, src.m_value);
}


int64_t Value::to_int64() const
{
    value::Int64 to_val;
    bool ok = to_val.cast_from(*this);
    assert(ok);
    return to_val.value();
}


StringV::StringV(std::string_view v)
        : slot(v.size() + sizeof(uint32_t))
{
    assert(v.size() < std::numeric_limits<uint32_t>::max());
    auto size = (uint32_t) v.size();
    std::memcpy(slot.data(), &size, sizeof(uint32_t));
    if (!v.empty())
        std::memcpy(slot.data() + sizeof(uint32_t), v.data(), v.size());
}


std::string_view StringV::value() const
{
    size_t size = bit_copy<uint32_t>(slot.data());
    const char* data = reinterpret_cast<const char*>(slot.data() + sizeof(uint32_t));
    return {data, size};
}


static void list_deleter(byte* data)
{
    auto length = bit_read<uint32_t>(data);
    auto deleter_data_size = bit_read<uint16_t>(data);
    if (length == 0 || deleter_data_size == 0)
        return;  // no slots to decref

    std::vector<size_t> offsets;
    offsets.reserve(8);
    auto* deleter_data_end = data + deleter_data_size;
    while (data < deleter_data_end)
        offsets.push_back(leb128_decode<size_t>(data));

    assert(!offsets.empty());
    size_t n_skip = offsets.back();
    offsets.pop_back();

    for (size_t el = 0; el != length; ++el) {
        for (size_t offset : offsets) {
            data += offset;
            // Read and decref the heap slot
            auto* slot_ptr = bit_copy<byte*>(data);
            HeapSlot{slot_ptr}.decref();
        }
        data += n_skip;
    }
}


ListV::ListV(size_t length, const TypeInfo& elem_type, const std::byte* elem_data)
{
    // prepare deleter data
    std::vector<size_t> offsets;
    offsets.reserve(8);
    elem_type.foreach_heap_slot([&offsets](size_t offset) {
        size_t last_offset = offsets.empty() ? 0 : offsets.back();
        offsets.push_back(offset - last_offset);
    });
    unsigned deleter_data_size = 0;
    if (!offsets.empty()) {
        // add final skip
        offsets.push_back(elem_type.size() - offsets.back());
        // sum of LEB128-encoded lengths of the offsets
        deleter_data_size = accumulate(offsets | transform([](size_t i){ return leb128_length(i); }), 0);
    }

    const size_t elem_data_size = length * elem_type.size();
    slot = HeapSlot(6 + deleter_data_size + elem_data_size, list_deleter);
    auto* data = slot.data();

    // length (number of elements)
    assert(length < std::numeric_limits<uint32_t>::max());
    bit_write(data, (uint32_t) length);

    // size of deleter data
    assert(deleter_data_size < std::numeric_limits<uint16_t>::max());
    bit_write(data, (uint16_t) deleter_data_size);

    // deleter data (offsets)
    if (deleter_data_size != 0) {
        for (auto ofs : offsets)
            leb128_encode(data, ofs);
    }

    // element data
    if (length != 0) {
        if (elem_data == nullptr)
            std::memset(data, 0, elem_data_size);
        else
            std::memcpy(data, elem_data, elem_data_size);
    }
}


size_t ListV::length() const
{
    return bit_copy<uint32_t>(slot.data());
}


Value ListV::value_at(size_t idx, const TypeInfo& elem_type) const
{
    assert(idx < length());
    const auto* data = slot.data() + sizeof(uint32_t);
    auto dd_size = bit_read<uint16_t>(data);
    data += dd_size;

    auto elem = create_value(elem_type);
    const auto elem_size = elem_type.size();
    elem.read(data + idx * elem_size);
    return elem;
}


TupleV::TupleV(const TupleV& other)
        : values(std::make_unique<Value[]>(other.length() + 1))
{
    for (int i = 0;;) {
        const Value& item = other.value_at(i);
        values[i++] = item;
        if (item.is_void())
            break;
    }
}


TupleV& TupleV::operator =(const TupleV& other)
{
    values = std::make_unique<Value[]>(other.length() + 1);
    for (int i = 0;;) {
        const Value& item = other.value_at(i);
        values[i++] = item;
        if (item.is_void())
            break;
    }
    return *this;
}


TupleV::TupleV(Values&& vs)
        : values(std::make_unique<Value[]>(vs.size() + 1))
{
    int i = 0;
    for (auto&& tv : vs) {
        values[i++] = move(tv);
    }
    values[i] = Value{};
}


TupleV::TupleV(const TypeInfo::Subtypes& subtypes)
        : values(std::make_unique<Value[]>(subtypes.size() + 1))
{
    int i = 0;
    for (const auto& ti : subtypes) {
        values[i++] = create_value(ti);
    }
    values[i] = Value{};
}


bool TupleV::empty() const
{
    return values[0].is_void();
}


size_t TupleV::length() const
{
    size_t i = 0;
    while (!values[i].is_void())
        ++i;
    return i;
}


void TupleV::foreach(const std::function<void(const Value&)>& cb) const
{
    for (Value* it = values.get(); !it->is_void(); ++it)
        cb(*it);
}


ClosureV::ClosureV(const Function& fn)
        : slot(sizeof(Function*))
{
    assert(fn.closure_size() == 0);
    const Function* fn_ptr = &fn;
    std::memcpy(slot.data(), &fn_ptr, sizeof(Function*));
}


ClosureV::ClosureV(const Function& fn, Values&& values)
        : slot(fn.raw_size_of_closure() + sizeof(Function*))
{
    assert(fn.closure_size() == values.size());
    value::Tuple closure(move(values));
    const Function* fn_ptr = &fn;
    std::memcpy(slot.data(), &fn_ptr, sizeof(Function*));
    closure.write(slot.data() + sizeof(Function*));
}


Function* ClosureV::function() const
{
    return bit_copy<Function*>(slot.data());
}


value::Tuple ClosureV::closure() const
{
    value::Tuple values{TypeInfo{function()->closure_types()}.subtypes()};
    values.read(slot.data() + sizeof(Function*));
    return values;
}


static void stream_deleter(byte* data)
{
    Stream v;
    v.raw_read(data);
    v.close();
}


StreamV::StreamV(const Stream& v)
        : slot(v.raw_size(), stream_deleter)
{
    v.raw_write(slot.data());
}


Stream StreamV::value() const
{
    Stream v;
    if (slot)
        v.raw_read(slot.data());
    return v;
}


size_t Values::size_on_stack() const
{
    return std::accumulate(m_items.begin(), m_items.end(), size_t(0),
        [](size_t init, const Value& value)
        { return init + value.size_on_stack(); });
}


TypedValue::TypedValue(Value value, TypeInfo type_info)
        : m_value(move(value)), m_type_info(move(type_info))
{
    assert(m_value.type() == m_type_info.type()
        || (m_value.type() == Type::Tuple && m_type_info.type() == Type::Struct));
}


// make sure float values don't look like integers (append .0 if needed)
static void dump_float(std::ostream& os, /*std::floating_point*/ auto value)
{
    std::ostringstream sbuf;
    sbuf << value;
    auto str = sbuf.str();
    if (str.find('.') == std::string::npos)
        os << str << ".0";
    else
        os << str;
}


class StreamVisitor: public value::Visitor {
public:
    explicit StreamVisitor(std::ostream& os, const TypeInfo& type_info = TypeInfo{}) : os(os), type_info(type_info) {}
    void visit(void) override { os << ""; }
    void visit(bool v) override { os << std::boolalpha << v; }
    void visit(std::byte v) override {
        os << "b'" << core::escape(core::to_utf8(uint8_t(v))) << "'";
    }
    void visit(char32_t v) override {
        os << '\'' << core::escape(core::to_utf8(v)) << "'";
    }
    void visit(int32_t v) override { os << v; }
    void visit(int64_t v) override { os << v << 'L'; }
    void visit(float v) override {
        dump_float(os, v);
        os << 'f';
    }
    void visit(double v) override {
        dump_float(os, v);
    }
    void visit(std::string_view&& v) override {
        os << '"' << core::escape(v) << '"';
    }
    void visit(const ListV& v) override {
        if (!type_info.is_unknown() && type_info.elem_type().type() == Type::Byte) {
            // special output for [Byte]
            const char* data = (const char*)v.slot.data() + 6;
            const std::string_view sv{data, v.length()};
            os << "b\"" << core::escape(sv) << '"';
            return;
        }
        os << "[";
        if (type_info.is_unknown()) {
            for (size_t idx = 0; idx < v.length(); idx++) {
                if (idx != 0)
                    os << ", ";
                os << '?';
            }
        } else {
            const auto& ti = type_info.elem_type();
            for (size_t idx = 0; idx < v.length(); idx++) {
                if (idx != 0)
                    os << ", ";
                os << TypedValue(v.value_at(idx, ti), ti);
            }
        }
        os << "]";
    }
    void visit(const TupleV& v) override {
        os << "(";
        if (type_info.type() == Type::Tuple) {
            auto ti_iter = type_info.subtypes().begin();
            for (Value* it = v.values.get(); !it->is_void(); ++it) {
                if (it != v.values.get())
                    os << ", ";
                os << TypedValue(*it, *ti_iter++);
            }
        } else if (type_info.type() == Type::Struct) {
            auto ti_iter = type_info.struct_items().begin();
            for (Value* it = v.values.get(); !it->is_void(); ++it) {
                if (it != v.values.get())
                    os << ", ";
                os << ti_iter->first << '=' << TypedValue(*it, ti_iter->second);
                ++ti_iter;
            }
        } else {
            // unknown TypeInfo
            for (Value* it = v.values.get(); !it->is_void(); ++it) {
                if (it != v.values.get())
                    os << ", ";
                os << *it;
            }
        }
        os << ")";
    }
    void visit(const ClosureV& v) override {
        const auto& fn = *v.function();
        os << fn.name() << ' ' << fn.signature();
        const auto& nonlocals = v.closure();
        if (!nonlocals.empty()) {
            auto closure_types = fn.closure_types();
            auto ti_iter = closure_types.begin();
            os << " (";
            nonlocals.tuple_foreach([&](const Value& item){
                if (ti_iter != closure_types.begin())
                    os << ", ";
                os << TypedValue(item, *ti_iter++);
            });
            os << ")";
        }
    }
    void visit(const script::Module* v) override {
        if (v == nullptr)
            fmt::print(os, "<module:null>");
        else
            fmt::print(os, "<module:{}>", v->name());
    }
    void visit(const script::Stream& v) override {
        os << "<stream:" << v << ">";
    }
private:
    std::ostream& os;
    const TypeInfo& type_info;
};


std::ostream& operator<<(std::ostream& os, const TypedValue& o)
{
    StreamVisitor visitor(os, o.type_info());
    o.value().apply(visitor);
    return os;
}


std::ostream& operator<<(std::ostream& os, const Value& o)
{
    StreamVisitor visitor(os);
    o.apply(visitor);
    return os;
}


namespace value {


Byte::Byte(std::string_view utf8)
{
    auto c = (uint32_t) core::utf8_codepoint(utf8.data());
    if (c > 255)
        throw std::runtime_error("byte value out of range");
    m_value = (byte) c;
}


Char::Char(std::string_view utf8)
    : Value(core::utf8_codepoint(utf8.data()))
{}


Bytes::Bytes(std::span<const std::byte> v)
        : List(v.size(), TypeInfo{Type::Byte})
{
    std::memcpy((void*)(heapslot()->data() + 6), v.data(), v.size());
}


} // namespace value

} // namespace xci::script
