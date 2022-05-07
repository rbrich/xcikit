// Stack.cpp created on 2019-05-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Stack.h"
#include "Error.h"
#include "Function.h"

#include <range/v3/view/reverse.hpp>

#include <iostream>
#include <iomanip>

namespace xci::script {

using ranges::cpp20::views::reverse;
using std::cout;
using std::endl;


void Stack::push(const Value& v)
{
    auto size = v.size_on_stack();
    if (size == 0)
        return;  // Void
    if (m_stack_pointer < size) {
        if (grow() < size)
            throw StackOverflow();
    }
    m_stack_pointer -= size;
    v.write(data());
    push_type(v);
}


Value Stack::pull(const TypeInfo& ti)
{
    if (ti.is_void())
        return Value();
    // create Value with TypeInfo, read contents from stack
    auto value = create_value(ti);
    pop_type(value);
    m_stack_pointer += value.read(data());
    return value;
}


Value Stack::get(StackRel pos, const TypeInfo& ti) const
{
    assert(pos + ti.size() <= size());
    auto value = create_value(ti);
    value.read(data() + pos);
    return value;
}


Value Stack::get(StackRel pos, Type type) const
{
    assert(pos + type_size_on_stack(type) <= size());
    auto value = create_value(type);
    value.read(data() + pos);
    return value;
}


void* Stack::get_ptr(StackRel pos) const
{
    void* value = nullptr;
    assert(pos + sizeof(value) <= size());
    std::memcpy(&value, data() + pos, sizeof(value));
    return value;
}


void Stack::clear_ptr(StackRel pos)
{
    assert(pos + sizeof(void*) <= size());
    std::memset(data() + pos, 0, sizeof(void*));
}


void Stack::copy(StackRel pos, size_t size)
{
    assert(pos + size <= Stack::size());
    // copy type(s) of the range
    size_t top_bytes = pos;
    auto it_type = m_stack_types.end();
    while (top_bytes > 0) {
        it_type --;
        auto type_size = type_size_on_stack(*it_type);
        assert(type_size <= top_bytes);
        top_bytes -= type_size;
    }
    size_t copy_bytes = size;
    std::vector<Type> copies;
    while (copy_bytes > 0) {
        it_type --;
        auto type_size = type_size_on_stack(*it_type);
        assert(copy_bytes >= type_size);
        copy_bytes -= type_size;
        copies.emplace_back(*it_type);
    }
    m_stack_types.insert(m_stack_types.end(), copies.rbegin(), copies.rend());
    // move stack pointer
    assert(size > 0);
    if (m_stack_pointer < size) {
        if (grow() < size)
            throw StackOverflow();
    }
    m_stack_pointer -= size;
    // copy the bytes
    memcpy(data(), data() + size + pos, size);
}


void Stack::drop(StackRel first, size_t size)
{
    assert(first + size <= Stack::size());
    // drop also m_stack_types, check type boundaries
    size_t top_bytes = 0;
    auto end_type = m_stack_types.end();
    while (top_bytes < first) {
        end_type --;
        top_bytes += type_size_on_stack(*end_type);
    }
    assert(top_bytes == first);
    size_t erase_bytes = size;
    auto begin_type = end_type;
    while (erase_bytes > 0) {
        begin_type --;
        auto type_size = type_size_on_stack(*begin_type);
        assert(erase_bytes >= type_size);
        erase_bytes -= type_size;
    }
    assert(erase_bytes == 0);
    m_stack_types.erase(begin_type, end_type);
    // remove the requested bytes
    memmove(data() + size, data(), first);
    m_stack_pointer += size;
}


void Stack::swap(size_t first, size_t second)
{
    assert(first + second <= Stack::size());
    // swap also m_stack_types, check type boundaries
    // first - types
    size_t type_bytes = 0;
    auto first_type_it = m_stack_types.end();
    while (type_bytes < first) {
        first_type_it --;
        type_bytes += type_size_on_stack(*first_type_it);
    }
    assert(type_bytes == first);
    // second - types
    type_bytes = 0;
    auto second_type_it = first_type_it;
    while (type_bytes < second) {
        second_type_it --;
        type_bytes += type_size_on_stack(*second_type_it);
    }
    assert(type_bytes == second);
    // swap types
    std::vector<Type> copies(second_type_it, first_type_it);
    m_stack_types.erase(second_type_it, first_type_it);
    m_stack_types.insert(m_stack_types.end(), copies.begin(), copies.end());
    // swap actual values
    // (only this is really needed, the above is just optional type info)
    auto tmp = std::make_unique<std::byte[]>(second);
    std::memcpy(tmp.get(), data() + first, second);  // copy second to tmp (skipping over first)
    std::memmove(data() + second, data(), first);  // move first (reserving space for second on top)
    std::memcpy(data(), tmp.get(), second);  // copy second from tmp
}


std::ostream& operator<<(std::ostream& os, const Stack& v)
{
    using std::right;
    using std::setw;

    Stack::StackRel pos = 0;
    auto frame = v.n_frames() - 1;
    auto base = v.to_rel(v.frame().base);
    auto check_print_base = [&] {
        // print frame boundary (only on exact match, note that
        // it may point to middle of a value after DROP)
        if (base == pos)
            cout << " --- ---  (frame " << frame << ")" << endl;
        // recompute base, frame for following stack values
        if (base <= pos && frame > 0)
            base = v.to_rel(v.frame(--frame).base);
    };
    // header
    cout << right << setw(4) << "pos" << setw(4) << "siz"
         << "  value" << endl;
    // stack data
    for (const auto type : reverse(v.m_stack_types)) {
        check_print_base();

        const auto size = type_size_on_stack(type);
        cout << setw(4) << right << pos;
        cout << setw(4) << right << size;

        auto value = v.get(pos, type);
        const auto* hs = value.heapslot();
        if (hs) {
            cout << "  heap:" << std::hex << (intptr_t) hs->data() << std::dec
                 << " refs:" << hs->refcount();
            if (*hs)
                cout << "  " << value << endl;
            else
                cout << endl;
        } else {
            cout << "  " << value << endl;
        }

        pos += size;
    }
    check_print_base();
    return os;
}


size_t Stack::grow()
{
    size_t newcap;
    if (m_stack_capacity < m_stack_max/2)
        newcap = m_stack_capacity * 2;
    else
        newcap = m_stack_capacity + m_stack_max/10;
    if (newcap > m_stack_max)
        newcap = m_stack_max;
    if (newcap == m_stack_capacity)
        // already at max size
        return m_stack_pointer;
    // copy old stack into new bigger stack
    auto newstack = std::make_unique<byte[]>(newcap);
    memcpy(newstack.get() + newcap - m_stack_capacity,
           m_stack.get(), m_stack_capacity);
    m_stack = std::move(newstack);
    m_stack_pointer += newcap - m_stack_capacity;
    m_stack_capacity = newcap;
    return m_stack_pointer;
}


void Stack::push_type(const Value& v)
{
    if (v.type() == Type::Tuple) {
        v.tuple_foreach([this](const Value& item){
            push_type(item);
        });
    } else
        m_stack_types.emplace_back(v.type());
}


void Stack::pop_type(const Value& v)
{
    if (Stack::size() < v.size_on_stack())
        throw StackUnderflow{};

    // check type(s) on stack
    if (v.type() == Type::Tuple) {
        v.tuple_foreach([this](const Value& item){
            pop_type(item);
        });
    } else {
        // allow casts - only size have to match
        assert(v.size_on_stack() == type_size_on_stack(top_type()));
        m_stack_types.pop_back();
    }
}


StackTrace Stack::make_trace()
{
    // unwind all variables on stack
    drop(0, size());
    // make trace from stack frames and clear them too
    StackTrace trace;
    while (!m_frame.empty()) {
        trace.push_back({
            frame().function.name()
        });
        pop_frame();
    }
    return trace;
}


} // namespace xci::script
