// Value.cpp created on 2019-05-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Value.h"
#include "Function.h"
#include "Module.h"
#include "Error.h"
#include "dump.h"
#include <xci/data/coding/leb128.h>
#include <xci/core/string.h>
#include <xci/core/template/helpers.h>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/range/conversion.hpp>
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
namespace views = ranges::cpp20::views;
using ranges::to;
using ranges::accumulate;


template<typename T>
concept HasHeapSlot = requires(const T& v) {
    v.slot;
    std::is_same_v<decltype(v.slot), HeapSlot>;
};


Value create_value(const TypeInfo& type_info)
{
    const auto type = type_info.type();
    switch (type) {
        case Type::List:
            if (type_info.elem_type() == TypeInfo{Type::UInt8})
                return value::Bytes();  // List subclass, with special output formatting
            else
                return value::List();
        case Type::Tuple:
            return value::Tuple{type_info.subtypes()};
        case Type::Struct: {
            auto subtypes = type_info.struct_items()
                | views::transform([](const TypeInfo::StructItem& item) { return item.second; })
                | to<std::vector>();
            return value::Tuple{subtypes};
        }
        case Type::Named:
            return create_value(type_info.named_type().type_info);
        default:
            return create_value(type);
    }
    return {};
}


Value create_value(Type type)
{
    switch (type) {
        case Type::Unknown: assert(!"Cannot create Value of Unknown type"); break;
        case Type::Bool: return value::Bool{};
        case Type::Char: return value::Char{};
        case Type::UInt8: return value::UInt8{};
        case Type::UInt16: return value::UInt16{};
        case Type::UInt32: return value::UInt32{};
        case Type::UInt64: return value::UInt64{};
        case Type::UInt128: return value::UInt128{};
        case Type::Int8: return value::Int8{};
        case Type::Int16: return value::Int16{};
        case Type::Int32: return value::Int32{};
        case Type::Int64: return value::Int64{};
        case Type::Int128: return value::Int128{};
        case Type::Float32: return value::Float32{};
        case Type::Float64: return value::Float64{};
        case Type::Float128: return value::Float128{};
        case Type::String: return value::String{};
        case Type::List: return value::List{};
        case Type::Tuple: return value::Tuple{};
        case Type::Struct: return value::Tuple{};  // struct differs only in type, otherwise it's just a tuple
        case Type::Function: return value::Closure{};
        case Type::Stream: return value::Stream{};
        case Type::Module: return value::Module{};
        case Type::TypeIndex: return value::TypeIndex{};
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
            return 0;  // Unknown
        else if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, byte>) {
            *buffer = byte(v);
            return 1;
        } else if constexpr (HasHeapSlot<T>) {
            const byte* slot_ptr = v.slot.slot();
            std::memcpy(buffer, &slot_ptr, sizeof(slot_ptr));
            return sizeof(byte*);
        } else if constexpr (std::is_same_v<T, TupleV>) {
            byte* ofs = buffer;
            for (Value* it = v.values.get(); !it->is_unknown(); ++it)
                ofs += it->write(ofs);
            return ofs - buffer;
        } else if constexpr (std::is_same_v<T, ModuleV>) {
            std::memcpy(buffer, &v.module_ptr, sizeof(v.module_ptr));
            return sizeof(v.module_ptr);
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
            return 0;  // Unknown
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
            for (Value* it = v.values.get(); !it->is_unknown(); ++it)
                ofs += it->read(ofs);
            return ofs - buffer;
        } else if constexpr (std::is_same_v<T, ModuleV>) {
            std::memcpy(&v.module_ptr, buffer, sizeof(v.module_ptr));
            return sizeof(v.module_ptr);
        } else if constexpr (std::is_same_v<T, TypeIndexV>) {
            std::memcpy(&v.type_index, buffer, sizeof(v.type_index));
            return sizeof(v.type_index);
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
            return 0;  // Unknown
        else if constexpr (std::is_same_v<T, TupleV>) {
            size_t res = 0;
            for (Value* it = v.values.get(); !it->is_unknown(); ++it)
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
#ifdef TRACE_REFCOUNT
    std::cout << "+ref " << *this << std::endl;
#endif
    return std::visit([](auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (HasHeapSlot<T>)
            v.slot.incref();
        else if constexpr (std::is_same_v<T, TupleV>) {
            for (Value* it = v.values.get(); !it->is_unknown(); ++it)
                it->incref();
        }
    }, m_value);
}


void Value::decref() const
{
#ifdef TRACE_REFCOUNT
    std::cout << "-ref " << *this << std::endl;
#endif
    return std::visit([](auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (HasHeapSlot<T>)
            v.slot.decref();
        else if constexpr (std::is_same_v<T, TupleV>) {
            for (Value* it = v.values.get(); !it->is_unknown(); ++it)
                it->decref();
        }
    }, m_value);
}


void Value::apply(value::Visitor& visitor) const
{
    std::visit([&visitor](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            assert(!"Cannot apply Value of unknown type");  // Unknown
        } else if constexpr (std::is_same_v<T, StringV> || std::is_same_v<T, StreamV>
                || std::is_same_v<T, ModuleV>)
            visitor.visit(v.value());
        else
            visitor.visit(v);
    }, m_value);
}


Type Value::type() const
{
    return std::visit(overloaded {
            [](std::monostate) { return Type::Unknown; },
            [](bool) { return Type::Bool; },
            [](char32_t) { return Type::Char; },
            [](uint8_t) { return Type::UInt8; },
            [](uint16_t) { return Type::UInt16; },
            [](uint32_t) { return Type::UInt32; },
            [](uint64_t) { return Type::UInt64; },
            [](uint128) { return Type::UInt128; },
            [](int8_t) { return Type::Int8; },
            [](int16_t) { return Type::Int16; },
            [](int32_t) { return Type::Int32; },
            [](int64_t) { return Type::Int64; },
            [](int128) { return Type::Int128; },
            [](float) { return Type::Float32; },
            [](double) { return Type::Float64; },
            [](float128) { return Type::Float128; },
            [](const StringV&) { return Type::String; },
            [](const ListV&) { return Type::List; },
            [](const TupleV&) { return Type::Tuple; },
            [](const ClosureV&) { return Type::Function; },
            [](const StreamV&) { return Type::Stream; },
            [](const ModuleV&) { return Type::Module; },
            [](const TypeIndexV&) { return Type::TypeIndex; },
    }, m_value);
}


bool Value::cast_from(const Value& src)
{
    return std::visit([](auto& to, const auto& from) -> bool {
        using TTo = std::decay_t<decltype(to)>;
        using TFrom = std::decay_t<decltype(from)>;

        // same types
        if constexpr (std::is_same_v<TFrom, TTo>) {
            to = from;
            return true;
        }

        // int/float -> int/float
        if constexpr
                ((std::is_integral_v<TFrom> || std::is_floating_point_v<TFrom>
                        || std::is_same_v<TFrom, byte> || std::is_same_v<TFrom, float128>)
                && (std::is_integral_v<TTo> || std::is_floating_point_v<TTo>
                        || std::is_same_v<TTo, byte> || std::is_same_v<TTo, float128>))
        {
            to = static_cast<TTo>(from);  // NOLINT(bugprone-signed-char-misuse)
            return true;
        }

        if  constexpr
                ((std::is_integral_v<TFrom> || std::is_same_v<TFrom, TypeIndexV>)
                 && (std::is_integral_v<TTo> || std::is_same_v<TTo, TypeIndexV>))
        {
            to = static_cast<TTo>(from);  // NOLINT(bugprone-signed-char-misuse)
            return true;
        }

        // Complex types
        return false;
    }, m_value, src.m_value);
}


bool Value::negate()
{
    return std::visit([](auto& v) -> bool {
        using T = std::decay_t<decltype(v)>;

        if constexpr ((std::is_integral_v<T> || std::is_floating_point_v<T>)
                && !std::is_same_v<T, bool>)
        {
            v = std::negate<>{}(v);
            return true;
        }

        return false;
    }, m_value);
}


int64_t Value::to_int64() const
{
    value::Int64 to_val;
    bool ok = to_val.cast_from(*this);
    assert(ok);
    (void) ok;
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
    if (!slot)
        return {nullptr, 0};
    size_t size = bit_copy<uint32_t>(slot.data());
    const char* data = reinterpret_cast<const char*>(slot.data() + sizeof(uint32_t));
    return {data, size};
}


template <typename InIter = byte*>
auto list_deleter_read_offsets(InIter& data, size_t size) -> std::vector<size_t>
{
    std::vector<size_t> offsets;
    InIter end = data + size;
    while (data < end)
        offsets.push_back(leb128_decode<size_t>(data));
    return offsets;
}

template <typename InIter = byte*, typename F>
void list_deleter_foreach_heap_slot(InIter& data, uint32_t length,
                                    const std::vector<size_t>& offsets, F&& cb)
{
   const size_t n_skip = offsets.back();

    for (size_t el = 0; el != length; ++el) {
        for (const size_t offset : offsets | views::take(offsets.size() - 1)) {
            data += offset;
            // Read and decref the heap slot
            auto* slot_ptr = bit_copy<byte*>(data);
            cb(HeapSlot{slot_ptr});
        }
        data += n_skip;
    }
}

static void list_deleter(byte* data)
{
    auto length = bit_read<uint32_t>(data);
    auto deleter_data_size = bit_read<uint16_t>(data);
    if (length == 0 || deleter_data_size == 0)
        return;  // no slots to decref

    const std::vector<size_t> offsets = list_deleter_read_offsets(data, deleter_data_size);
    assert(!offsets.empty());  // last offset skips the whole element - must be present
    list_deleter_foreach_heap_slot(data, length, offsets,
            [](HeapSlot&& slot){ slot.decref(); });
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
        deleter_data_size = accumulate(offsets | views::transform([](size_t i){ return leb128_length(i); }), 0);
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


const std::byte* ListV::raw_data() const
{
    const auto* data = slot.data() + sizeof(uint32_t);
    auto dd_size = bit_read<uint16_t>(data);
    data += dd_size;
    return data;
}


Value ListV::value_at(size_t idx, const TypeInfo& elem_type) const
{
    assert(idx < length());
    auto elem = create_value(elem_type);
    const auto elem_size = elem_type.size();
    elem.read(raw_data() + idx * elem_size);
    return elem;
}


void ListV::set_value(size_t idx, const Value& v)
{
    assert(idx < length());
    const auto elem_size = v.size_on_stack();
    v.write(raw_data() + idx * elem_size);
}


void ListV::slice(int64_t begin, int64_t end, int64_t step, const TypeInfo& elem_type)
{
    const auto* data = slot.data();
    auto length = (int32_t) bit_read<uint32_t>(data);

    // deleter data
    const auto offsets_size = bit_read<uint16_t>(data);
    const auto offsets = list_deleter_read_offsets(data, offsets_size);

    // adjust indexes
    if (end < 0)
        end += length;  // -1 => length - 1
    if (begin < 0 && begin != std::numeric_limits<int64_t>::min())
        begin += length;  // -1 => length - 1

    // compute number of elements in the sliced list
    unsigned n_sliced = 0;
    if (step > 0) {
        if (begin == std::numeric_limits<int64_t>::min()) {
            begin = 0;
        } else if (begin < 0) {
            // get closer to zero if still negative
            begin %= step;
            // step over zero if still negative
            if (begin < 0)
                begin += step;
        }
        if (end > length)
            end = length;
        auto i = begin;
        while (i < end) {
            ++ n_sliced;
            if (i > std::numeric_limits<int64_t>::max() - step)
                break;
            i += step;
        }
    } else if (step < 0) {
        if (begin == std::numeric_limits<int64_t>::max()) {
            begin = length - 1;
        } else if (begin >= length) {
            // get closer to length
            begin = (begin - length) % step;
            if (begin >= 0)
                begin += step;
            begin += length;
        }
        if (end < -1)
            end = -1;
        auto i = begin;
        while (i > end) {
            ++ n_sliced;
            if (i < std::numeric_limits<int64_t>::min() - step)
                break;
            i += step;
        }
    } else {
        assert(step == 0);
        if (begin >= 0 && begin < end)
            n_sliced = 1;
    }

    const auto elem_size = elem_type.size();

/*
    if (slot.refcount() == 1) {
        // TODO: in-place
    }
*/

    HeapSlot new_slot(6 + offsets_size + n_sliced * elem_size, list_deleter);
    auto* new_data = new_slot.data();
    bit_write(new_data, (uint32_t) n_sliced);
    bit_write(new_data, (uint16_t) offsets_size);
    std::memcpy(new_data, slot.data() + sizeof(uint32_t) + sizeof(uint16_t), offsets_size);
    new_data += offsets_size;

    if (step > 0) {
        auto orig_i = begin;
        auto* new_ptr = new_data;
        while (orig_i < end) {
            std::memcpy(new_ptr, data + orig_i * elem_size, elem_size);
            new_ptr += elem_size;
            if (orig_i > std::numeric_limits<int64_t>::max() - step)
                break;
            orig_i += step;
        }
    } else if (step < 0) {
        auto orig_i = begin;
        auto* new_ptr = new_data;
        while (orig_i > end) {
            std::memcpy(new_ptr, data + orig_i * elem_size, elem_size);
            new_ptr += elem_size;
            if (orig_i < std::numeric_limits<int64_t>::min() - step)
                break;
            orig_i += step;
        }
    } else {
        assert(step == 0);
        if (begin >= 0 && begin < end) {
            std::memcpy(new_data, data + begin * elem_size, elem_size);
        }
    }

    // incref all new elements
    if (!offsets.empty()) {
        list_deleter_foreach_heap_slot(new_data, n_sliced, offsets,
                [](HeapSlot&& heap_slot){ heap_slot.incref(); });
    }

    // decref original slot (all elements) and replace it
    slot.decref();
    slot = new_slot;
}


void ListV::extend(const ListV& rhs, const TypeInfo& elem_type)
{
    const auto* data1 = slot.data();
    auto length1 = (int32_t) bit_read<uint32_t>(data1);
    const auto offsets_size = bit_read<uint16_t>(data1);
    const auto offsets = list_deleter_read_offsets(data1, offsets_size);
    const auto* data2 = rhs.slot.data();
    auto length2 = (int32_t) bit_read<uint32_t>(data2);
    data2 += 2 + offsets_size;

    // prepare new list
    const auto elem_size = elem_type.size();
    const auto new_length = length1 + length2;
    HeapSlot new_slot(6 + offsets_size + new_length * elem_size, list_deleter);
    auto* new_data = new_slot.data();

    bit_write(new_data, (uint32_t) new_length);
    bit_write(new_data, (uint16_t) offsets_size);
    std::memcpy(new_data, slot.data() + 6, offsets_size);
    new_data += offsets_size;

    std::memcpy(new_data, data1, length1 * elem_size);
    new_data += length1 * elem_size;
    std::memcpy(new_data, data2, length2 * elem_size);

    if (slot.refcount() == 1) {
        // in-place: incref only other, do not touch refs of this
        if (!offsets.empty()) {
            list_deleter_foreach_heap_slot(data2, length2, offsets,
                               [](HeapSlot&& heap_slot){ heap_slot.incref(); });
        }
        slot.release();
    } else {
        // incref all copied elements
        if (!offsets.empty()) {
            list_deleter_foreach_heap_slot(data1, length1, offsets,
                               [](HeapSlot&& heap_slot){ heap_slot.incref(); });
            list_deleter_foreach_heap_slot(data2, length2, offsets,
                               [](HeapSlot&& heap_slot){ heap_slot.incref(); });
        }
        slot.decref();
    }
    slot = new_slot;
}


TupleV::TupleV(const TupleV& other)
        : values(std::make_unique<Value[]>(other.length() + 1))
{
    for (int i = 0;;) {
        const Value& item = other.value_at(i);
        values[i++] = item;
        if (item.is_unknown())
            break;
    }
}


TupleV& TupleV::operator =(const TupleV& other)
{
    values = std::make_unique<Value[]>(other.length() + 1);
    for (int i = 0;;) {
        const Value& item = other.value_at(i);
        values[i++] = item;
        if (item.is_unknown())
            break;
    }
    return *this;
}


TupleV::TupleV(Values&& vs)
        : values(std::make_unique<Value[]>(vs.size() + 1))
{
    int i = 0;
    for (auto&& tv : vs) {
        values[i++] = std::move(tv);
    }
    values[i] = Value();
}


TupleV::TupleV(const TypeInfo::Subtypes& subtypes)
        : values(std::make_unique<Value[]>(subtypes.size() + 1))
{
    int i = 0;
    for (const auto& ti : subtypes) {
        values[i++] = create_value(ti);
    }
    values[i] = Value();
}


bool TupleV::empty() const
{
    return values[0].is_unknown();
}


size_t TupleV::length() const
{
    size_t i = 0;
    while (!values[i].is_unknown())
        ++i;
    return i;
}


void TupleV::foreach(const std::function<void(const Value&)>& cb) const
{
    for (Value* it = values.get(); !it->is_unknown(); ++it)
        cb(*it);
}


ClosureV::ClosureV(const Function& fn)
        : slot(sizeof(Function*))
{
    assert(fn.nonlocals().size() == 0);
    const Function* fn_ptr = &fn;
    std::memcpy(slot.data(), &fn_ptr, sizeof(Function*));
}


ClosureV::ClosureV(const Function& fn, Values&& values)
        : slot(fn.raw_size_of_nonlocals() + sizeof(Function*))
{
    assert(fn.nonlocals().size() == values.size());
    value::Tuple closure(std::move(values));
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
    value::Tuple values{function()->nonlocals()};
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
        : m_value(std::move(value)), m_type_info(std::move(type_info))
{
    assert(m_value.type() == m_type_info.underlying().type()
        || (m_value.type() == Type::Tuple && m_type_info.underlying().is_struct()));
}


// make sure float values don't look like integers (append .0 if needed)
static std::ostream& dump_float(std::ostream& os, /*std::floating_point*/ auto value)
{
    std::ostringstream sbuf;
    sbuf << std::setprecision(std::numeric_limits<decltype(value)>::digits10) << value;
    auto str = sbuf.str();
    if (str.find_first_of(".en") == std::string::npos)  // 'n' is for "inf" and "nan"
        return os << str << ".0";
    else
        return os << str;
}


class StreamVisitor: public value::Visitor {
public:
    explicit StreamVisitor(std::ostream& os, const TypeInfo& type_info) : os(os), type_info(type_info) {}
    void visit(bool v) override { os << std::boolalpha << v; }
    void visit(char32_t v) override {
        os << '\'' << core::escape_utf8(core::to_utf8(v)) << "'";
    }
    void visit(uint8_t v) override { os << unsigned(v) << "b"; }
    void visit(uint16_t v) override { os << v << "uh"; }
    void visit(uint32_t v) override { os << v << "ud"; }
    void visit(uint64_t v) override { os << v << "u"; }
    void visit(uint128 v) override { os << uint128_to_string(v) << "uq"; }
    void visit(int8_t v) override { os << int(v) << "c"; }
    void visit(int16_t v) override { os << v << "h"; }
    void visit(int32_t v) override { os << v << "d"; }
    void visit(int64_t v) override { os << v; }
    void visit(int128 v) override { os << int128_to_string(v) << "q"; }
    void visit(float v) override { dump_float(os, v) << 'f'; }
    void visit(double v) override { dump_float(os, v); }
    void visit(float128 v) override { dump_float(os, v) << 'q'; }
    void visit(std::string_view&& v) override {
        os << '"' << core::escape_utf8(v) << '"';
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
        if (type_info.is_named())
            os << type_info.name();
        const auto& underlying = type_info.underlying();
        os << '(';
        if (underlying.is_tuple()) {
            auto ti_iter = underlying.subtypes().begin();
            for (Value* it = v.values.get(); !it->is_unknown(); ++it) {
                if (it != v.values.get())
                    os << ", ";
                os << TypedValue(*it, *ti_iter++);
            }
        } else if (underlying.is_struct()) {
            auto ti_iter = underlying.struct_items().begin();
            for (Value* it = v.values.get(); !it->is_unknown(); ++it) {
                if (it != v.values.get())
                    os << ", ";
                os << ti_iter->first << '=' << TypedValue(*it, ti_iter->second);
                ++ti_iter;
            }
        } else {
            // unknown TypeInfo
            for (Value* it = v.values.get(); !it->is_unknown(); ++it) {
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
            auto ti_iter = fn.nonlocals().begin();
            os << " (";
            nonlocals.tuple_foreach([&](const Value& item){
                if (ti_iter != fn.nonlocals().begin())
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
    void visit(const TypeIndexV& v) override {
        os << "<type_index:";
        if (v.value() == no_index)
            os << "no_index";
        else {
            const auto module_index = v.value() % Index(128);
            const auto type_index = v.value() / Index(128);
            os << module_index << '/' << type_index;
        }
        //<< v.value()
        os << ">";
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
    TypeInfo type_info;  // Unknown
    StreamVisitor visitor(os, type_info);
    o.apply(visitor);
    return os;
}


namespace value {


UInt8::UInt8(std::string_view str)
{
    if (str.size() != 1)
        throw value_out_of_range("Byte value out of range");
    m_value = (uint8_t) str.front();
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
