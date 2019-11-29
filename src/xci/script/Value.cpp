// Value.cpp created on 2019-05-18, part of XCI toolkit
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

#include "Value.h"
#include "Function.h"
#include <xci/core/string.h>
#include <numeric>

namespace xci::script {

using namespace std;


std::unique_ptr<Value> Value::create(const TypeInfo& type_info)
{
    switch (type_info.type()) {
        case Type::Unknown: assert(!"Cannot create Value of Unknown type"); break;
        case Type::Void: return std::make_unique<value::Void>();
        case Type::Bool: return std::make_unique<value::Bool>();
        case Type::Byte: return std::make_unique<value::Byte>();
        case Type::Char: return std::make_unique<value::Char>();
        case Type::Int32: return std::make_unique<value::Int32>();
        case Type::Int64: return std::make_unique<value::Int64>();
        case Type::Float32: return std::make_unique<value::Float32>();
        case Type::Float64: return std::make_unique<value::Float64>();
        case Type::String: return std::make_unique<value::String>();
        case Type::List: return std::make_unique<value::List>(type_info.elem_type());
        case Type::Tuple: return std::make_unique<value::Tuple>(type_info);
        case Type::Function: return std::make_unique<value::Lambda>();
        case Type::Module: return std::make_unique<value::Module>();
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
    return std::accumulate(m_items.begin(), m_items.end(), 0,
        [](size_t init, const unique_ptr<Value>& value)
        { return init + value->size(); });
}


std::ostream& operator<<(std::ostream& os, const Value& o)
{
    struct StreamVisitor: public value::Visitor {
        std::ostream& os;
        explicit StreamVisitor(std::ostream& os) : os(os) {}
        void visit(const value::Void&) override { os << "Void"; }
        void visit(const value::Bool& v) override { os << std::boolalpha << v.value(); }
        void visit(const value::Byte& v) override { os << v.value() << ":Byte"; }
        void visit(const value::Char& v) override { os << v.value() << ":Char"; }
        void visit(const value::Int32& v) override { os << v.value(); }
        void visit(const value::Int64& v) override { os << v.value() << ":Int64"; }
        void visit(const value::Float32& v) override { os << v.value(); }
        void visit(const value::Float64& v) override { os << v.value() << ":Float64"; }
        void visit(const value::String& v) override { os << '"' << core::escape(v.value()) << '"'; }
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
        void visit(const value::Lambda& v) override {
            auto closure = v.closure();
            const auto& nonlocals = closure.values();
            os << "<function> " << v.function().signature() << " (";
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


String::String(const std::string& v)
    : m_size(v.size()), m_value(v.size())
{
    std::memcpy(m_value.data(), v.data(), v.size());
}


String::String(const char* data, size_t size)
    : m_size(size), m_value(size)
{
    std::memcpy(m_value.data(), data, size);
}


String& String::operator=(String&& rhs) noexcept
{
    std::swap(m_size, rhs.m_size);
    std::swap(m_value, rhs.m_value);
    return *this;
}


void String::write(byte* buffer) const
{
    const byte* slot = m_value.slot();
    std::memcpy(buffer, &slot, sizeof(slot));
    std::memcpy(buffer + sizeof(slot), &m_size, sizeof(m_size));
    m_value.incref();
}


void String::read(const byte* buffer)
{
    byte *slot = nullptr;
    std::memcpy(&slot, buffer, sizeof(slot));
    std::memcpy(&m_size, buffer + sizeof(slot), sizeof(m_size));
    m_value.reset(slot);
}


List::List(const Values& values)
    : m_length(values.size())
{
    if (values.empty())
        return;  // default initialized empty list
    m_elem_type = values[0].type_info();
    auto elem_size = m_elem_type.size();
    m_elements.reset(m_length * elem_size);
    auto* dataptr = m_elements.data();
    for (const auto& value : values) {
        if (value->type_info() != m_elem_type)
            throw ListElemTypeMismatch(m_elem_type, value->type_info());
        value->write(dataptr);
        dataptr += elem_size;
    }
}

void List::write(byte* buffer) const
{
    const byte* slot = m_elements.slot();
    std::memcpy(buffer, &slot, sizeof(slot));
    std::memcpy(buffer + sizeof(slot), &m_length, sizeof(m_length));
    m_elements.incref();
}


void List::read(const byte* buffer)
{
    byte *slot = nullptr;
    std::memcpy(&slot, buffer, sizeof(slot));
    std::memcpy(&m_length, buffer + sizeof(slot), sizeof(m_length));
    m_elements.reset(slot);
}


std::unique_ptr<Value> List::get(size_t idx) const
{
    assert(idx < m_length);
    auto elem = Value::create(m_elem_type);
    elem->read(m_elements.data() + idx * m_elem_type.size());
    return elem;
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


Lambda::Lambda(Function& v) : m_function(&v) {}


Lambda::Lambda(Function& v, Values&& nonlocals)
        : m_function(&v), m_closure(v.raw_size_of_nonlocals())
{
    assert(m_function->nonlocals().size() == nonlocals.size());
    Tuple closure(move(nonlocals));
    closure.write(m_closure.data());
}


std::unique_ptr<Value> Lambda::make_copy() const
{
    return std::make_unique<Lambda>(*m_function, m_closure);
}


void Lambda::write(byte* buffer) const
{
    const byte* slot = m_closure.slot();
    std::memcpy(buffer, &slot, sizeof(slot));
    std::memcpy(buffer + sizeof(slot), &m_function, sizeof(m_function));
    m_closure.incref();
}


void Lambda::read(const byte* buffer)
{
    byte *slot = nullptr;
    std::memcpy(&slot, buffer, sizeof(slot));
    std::memcpy(&m_function, buffer + sizeof(slot), sizeof(m_function));
    m_closure.reset(slot);
}


TypeInfo Lambda::type_info() const
{
    if (m_function == nullptr)
        return TypeInfo{Type::Function};
    return TypeInfo{m_function->signature_ptr()};
}


Tuple Lambda::closure() const
{
    Tuple closure{TypeInfo{m_function->nonlocals()}};
    closure.read(m_closure.data());
    return closure;
}


} // namespace value

} // namespace xci::script
