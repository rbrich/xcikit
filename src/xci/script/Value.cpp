// Value.cpp created on 2019-05-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Value.h"
#include "Function.h"
#include "Error.h"
#include <xci/core/string.h>
#include <xci/core/log.h>
#include <xci/core/template/helpers.h>
#include <numeric>
#include <sstream>
#include <limits>

namespace xci::script {

using namespace xci::core;
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
    size_t size = bit_read<uint32_t>(slot.data());
    const char* data = reinterpret_cast<const char*>(slot.data() + sizeof(uint32_t));
    return {data, size};
}


ListV::ListV(size_t length, const TypeInfo& elem_type)
        : slot(length * elem_type.size() + sizeof(uint32_t))
{
    assert(length < std::numeric_limits<uint32_t>::max());
    auto len_raw = (uint32_t) length;
    std::memcpy(slot.data(), &len_raw, sizeof(uint32_t));
    if (length != 0)
        std::memset(slot.data() + sizeof(uint32_t), 0, length * elem_type.size());
}


size_t ListV::length() const
{
    return bit_read<uint32_t>(slot.data());
}


Value ListV::value_at(size_t idx, const TypeInfo& elem_type) const
{
    assert(idx < length());
    auto elem = create_value(elem_type);
    const auto elem_size = elem_type.size();
    elem.read(slot.data() + sizeof(uint32_t) + idx * elem_size);
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
    return bit_read<Function*>(slot.data());
}


value::Tuple ClosureV::closure() const
{
    value::Tuple values{TypeInfo{function()->closure_types()}.subtypes()};
    values.read(slot.data() + sizeof(Function*));
    return values;
}


static void stream_deleter(const byte* data)
{
    Stream v;
    v.raw_read(data);
    v.close();
}


StreamV::StreamV(const Stream& v)
        : slot(Stream::raw_size(), stream_deleter)
{
    v.raw_write(slot.data());
}


Stream StreamV::value() const
{
    Stream v;
    v.raw_read(slot.data());
    return v;
}


size_t Values::size_on_stack() const
{
    return std::accumulate(m_items.begin(), m_items.end(), size_t(0),
        [](size_t init, const Value& value)
        { return init + value.size_on_stack(); });
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
            const char* data = (const char*)v.slot.data() + sizeof(uint32_t);
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
        if (type_info.is_unknown()) {
            for (Value* it = v.values.get(); !it->is_void(); ++it) {
                if (it != v.values.get())
                    os << ", ";
                os << Value(*it);
            }
        } else {
            auto ti_iter = type_info.subtypes().begin();
            for (Value* it = v.values.get(); !it->is_void(); ++it) {
                if (it != v.values.get())
                    os << ", ";
                os << TypedValue(Value(*it), *ti_iter++);
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
    void visit(const script::Module* v) override { fmt::print(os, "<module:{:x}>", uintptr_t(v)); }
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
    std::memcpy((void*)(heapslot()->data() + sizeof(uint32_t)),
            v.data(), v.size());
}


} // namespace value

} // namespace xci::script
