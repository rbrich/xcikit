// Value.cpp created on 2019-05-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Value.h"
#include "Function.h"
#include "Error.h"
#include <xci/core/string.h>
#include <numeric>
#include <sstream>

namespace xci::script {

using std::move;
using std::unique_ptr;
using std::make_unique;


std::unique_ptr<Value> Value::create(const TypeInfo& type_info)
{
    switch (type_info.type()) {
        case Type::Unknown: assert(!"Cannot create Value of Unknown type"); break;
        case Type::Void: return make_unique<value::Void>();
        case Type::Bool: return make_unique<value::Bool>();
        case Type::Byte: return make_unique<value::Byte>();
        case Type::Char: return make_unique<value::Char>();
        case Type::Int32: return make_unique<value::Int32>();
        case Type::Int64: return make_unique<value::Int64>();
        case Type::Float32: return make_unique<value::Float32>();
        case Type::Float64: return make_unique<value::Float64>();
        case Type::String: return make_unique<value::String>();
        case Type::List:
            if (type_info.elem_type() == TypeInfo{Type::Byte})
                return make_unique<value::Bytes>();  // List subclass, with special output formatting
            else
                return make_unique<value::List>(type_info.elem_type());
        case Type::Tuple: return make_unique<value::Tuple>(type_info);
        case Type::Function: return make_unique<value::Closure>();
        case Type::Module: return make_unique<value::Module>();
    }
    return nullptr;
}


Values::Values(const Values& other)
{
    for (const auto& val : other)
        add(val->make_copy());
}


Values& Values::operator=(const Values& rhs)
{
    m_items.clear();
    for (const auto& val : rhs)
        add(val->make_copy());
    return *this;
}


size_t Values::raw_size() const
{
    return std::accumulate(m_items.begin(), m_items.end(), size_t(0),
        [](size_t init, const unique_ptr<Value>& value)
        { return init + value->size(); });
}


// make sure float values don't look like integers (append .0 if needed)
static void dump_float(std::ostream& os, auto value)
{
    std::ostringstream sbuf;
    sbuf << value;
    auto str = sbuf.str();
    if (str.find('.') == std::string::npos)
        os << str << ".0";
    else
        os << str;
}


std::ostream& operator<<(std::ostream& os, const Value& o)
{
    struct StreamVisitor: public value::Visitor {
        std::ostream& os;
        explicit StreamVisitor(std::ostream& os) : os(os) {}
        void visit(const value::Void&) override { os << ""; }
        void visit(const value::Bool& v) override { os << std::boolalpha << v.value(); }
        void visit(const value::Byte& v) override {
            os << "b'" << core::escape(core::to_utf8(v.value())) << "'";
        }
        void visit(const value::Char& v) override {
            os << '\'' << core::escape(core::to_utf8(v.value())) << "'";
        }
        void visit(const value::Int32& v) override { os << v.value(); }
        void visit(const value::Int64& v) override { os << v.value() << 'L'; }
        void visit(const value::Float32& v) override {
            dump_float(os, v.value());
            os << 'f';
        }
        void visit(const value::Float64& v) override {
            dump_float(os, v.value());
        }
        void visit(const value::Bytes& v) override {
            const std::string_view sv{(const char*)v.value().data(), v.value().size()};
            os << "b\"" << core::escape(sv) << '"';
        }
        void visit(const value::String& v) override {
            os << '"' << core::escape(v.value()) << '"';
        }
        void visit(const value::List& v) override {
            os << "[";
            for (size_t idx = 0; idx < v.length(); idx++) {
                os << *v.get(idx);
                if (idx < v.length() - 1)
                    os << ", ";
            }
            os << "]";
        }
        void visit(const value::Tuple& v) override {
            const auto& values = v.values();
            os << "(";
            for (const auto& item : values) {
                os << *item;
                if (item.get() != values.back().get())
                    os << ", ";
            }
            os << ")";
        }
        void visit(const value::Closure& v) override {
            auto closure = v.closure();
            const auto& nonlocals = closure.values();
            os << v.function().name() << " (";
            for (const auto& item : nonlocals) {
                os << *item;
                if (item.get() != nonlocals.back().get())
                    os << ", ";
            }
            os << ")";
        }
        void visit(const value::Module& v) override { os << "<module>"; }
    };
    StreamVisitor v(os);
    o.apply(v);
    return os;
}


namespace value {


Byte::Byte(std::string_view utf8)
{
    auto c = (uint32_t) core::utf8_codepoint(utf8.data());
    if (c > 255)
        throw std::runtime_error("byte value out of range");
    m_value = (uint8_t) c;
}


Char::Char(std::string_view utf8)
    : m_value(core::utf8_codepoint(utf8.data()))
{}


String::String(std::string_view v)
    : m_size(v.size()), m_value(v.size())
{
    std::memcpy(m_value.data(), v.data(), v.size());
}


String& String::operator=(String&& rhs) noexcept
{
    std::swap(m_size, rhs.m_size);
    std::swap(m_value, rhs.m_value);
    return *this;
}


std::unique_ptr<Value> String::make_copy() const
{
    return std::make_unique<String>(m_size, m_value);
}


void String::write(byte* buffer) const
{
    const byte* slot = m_value.slot();
    std::memcpy(buffer, &slot, sizeof(slot));
    std::memcpy(buffer + sizeof(slot), &m_size, sizeof(m_size));
}


void String::read(const byte* buffer)
{
    byte *slot = nullptr;
    std::memcpy(&slot, buffer, sizeof(slot));
    std::memcpy(&m_size, buffer + sizeof(slot), sizeof(m_size));
    m_value = HeapSlot{slot};
}


List::List(const Values& values)
    : m_length(values.size())
{
    if (values.empty())
        return;  // default initialized empty list
    m_elem_type = values[0].type_info();
    auto elem_size = m_elem_type.size();
    m_elements = HeapSlot{m_length * elem_size};
    auto* dataptr = m_elements.data();
    for (const auto& value : values) {
        if (value->type_info() != m_elem_type)
            throw ListElemTypeMismatch(m_elem_type, value->type_info());
        value->write(dataptr);
        dataptr += elem_size;
    }
}

std::unique_ptr<Value> List::make_copy() const
{
    return std::make_unique<List>(m_elem_type, m_length, m_elements);
}


void List::write(byte* buffer) const
{
    const byte* slot = m_elements.slot();
    std::memcpy(buffer, &slot, sizeof(slot));
    std::memcpy(buffer + sizeof(slot), &m_length, sizeof(m_length));
}


void List::read(const byte* buffer)
{
    byte *slot = nullptr;
    std::memcpy(&slot, buffer, sizeof(slot));
    std::memcpy(&m_length, buffer + sizeof(slot), sizeof(m_length));
    m_elements = HeapSlot{slot};
}


std::unique_ptr<Value> List::get(size_t idx) const
{
    assert(idx < m_length);
    auto elem = Value::create(m_elem_type);
    elem->read(m_elements.data() + idx * m_elem_type.size());
    return elem;
}


Bytes::Bytes(std::span<const std::byte> v)
        : List(TypeInfo{Type::Byte}, v.size(), HeapSlot{v.size()})
{
    std::memcpy(heapslot_ref().data(), v.data(), v.size());
}


Tuple::Tuple(const TypeInfo& type_info)
{
    for (const TypeInfo& subtype : type_info.subtypes()) {
        m_values.add(Value::create(subtype));
    }
}


void Tuple::write(byte* buffer) const
{
    for (const auto& v : m_values) {
        v->write(buffer);
        buffer += v->size();
    }
}


void Tuple::read(const byte* buffer)
{
    for (auto& v : m_values) {
        v->read(buffer);
        buffer += v->size();
    }
}


void Tuple::incref() const
{
    for (auto& v : m_values) {
        v->incref();
    }
}


void Tuple::decref() const
{
    for (auto& v : m_values) {
        v->decref();
    }
}


TypeInfo Tuple::type_info() const
{
    // build TypeInfo from subtypes
    std::vector<TypeInfo> subtypes;
    subtypes.reserve(m_values.size());
    for (auto& v : m_values) {
        subtypes.push_back(v->type_info());
    }
    return TypeInfo(move(subtypes));
}


Closure::Closure(Function& v) : m_function(&v) {}


Closure::Closure(Function& v, Values&& values)
        : m_function(&v), m_closure(v.raw_size_of_closure())
{
    assert(m_function->closure_size() == values.size());
    Tuple closure(move(values));
    closure.write(m_closure.data());
}


std::unique_ptr<Value> Closure::make_copy() const
{
    return std::make_unique<Closure>(*m_function, m_closure);
}


void Closure::write(byte* buffer) const
{
    const byte* slot = m_closure.slot();
    std::memcpy(buffer, &slot, sizeof(slot));
    std::memcpy(buffer + sizeof(slot), &m_function, sizeof(m_function));
}


void Closure::read(const byte* buffer)
{
    byte *slot = nullptr;
    std::memcpy(&slot, buffer, sizeof(slot));
    std::memcpy(&m_function, buffer + sizeof(slot), sizeof(m_function));
    m_closure = HeapSlot{slot};
}


TypeInfo Closure::type_info() const
{
    if (m_function == nullptr)
        return TypeInfo{Type::Function};
    return TypeInfo{m_function->signature_ptr()};
}


Tuple Closure::closure() const
{
    Tuple closure{TypeInfo{m_function->closure()}};
    closure.read(m_closure.data());
    return closure;
}


} // namespace value

} // namespace xci::script
