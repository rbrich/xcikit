// Stack.cpp created on 2019-05-18, part of XCI toolkit
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

#include "Stack.h"
#include "Error.h"

#include <range/v3/view/reverse.hpp>

#include <iomanip>

namespace xci::script {

using ranges::cpp20::views::reverse;
using std::move;


void Stack::push(const Value& o)
{
    auto ti = o.type_info();
    auto size = ti.size();
    assert(size > 0);
    if (m_stack_pointer < size) {
        if (grow() < size)
            throw StackOverflow();
    }
    m_stack_pointer -= size;
    o.write(&m_stack[m_stack_pointer]);
    m_stack_types.emplace_back(move(ti));
}


std::unique_ptr<Value> Stack::pull(const TypeInfo& ti)
{
    auto s = ti.size();
    pop_type(ti, s);
    // create Value with TypeInfo, read contents from stack
    auto value = Value::create(ti);
    value->read(&m_stack[m_stack_pointer]);
    m_stack_pointer += s;
    return value;
}


std::unique_ptr<Value> Stack::get(StackRel pos, const TypeInfo& ti) const
{
    assert(pos + ti.size() <= size());
    auto value = Value::create(ti);
    value->read(&m_stack[m_stack_pointer + pos]);
    return value;
}


void* Stack::get_ptr(Stack::StackRel pos) const
{
    void* value = nullptr;
    assert(pos + sizeof(value) <= size());
    std::memcpy(&value, &m_stack[m_stack_pointer + pos], sizeof(value));
    return value;
}


void Stack::clear_ptr(StackRel pos)
{
    assert(pos + sizeof(void*) <= size());
    std::memset(&m_stack[m_stack_pointer + pos], 0, sizeof(void*));
}


void Stack::copy(StackRel pos, size_t size)
{
    assert(pos + size <= Stack::size());
    // copy type(s) of the range
    size_t top_bytes = pos;
    auto it_type = m_stack_types.end();
    while (top_bytes > 0) {
        it_type --;
        auto type_size = it_type->size();
        assert(type_size <= top_bytes);
        top_bytes -= type_size;
    }
    size_t copy_bytes = size;
    std::vector<TypeInfo> copies;
    while (copy_bytes > 0) {
        it_type --;
        auto type_size = it_type->size();
        assert(copy_bytes >= type_size);
        copy_bytes -= type_size;
        copies.emplace_back(*it_type);
    }
    m_stack_types.insert(m_stack_types.end(), copies.begin(), copies.end());
    // move stack pointer
    assert(size > 0);
    if (m_stack_pointer < size) {
        if (grow() < size)
            throw StackOverflow();
    }
    m_stack_pointer -= size;
    // copy the bytes
    memcpy(&m_stack[m_stack_pointer],
           &m_stack[m_stack_pointer + size + pos],
           size);
}


void Stack::drop(StackRel first, size_t size)
{
    assert(first + size <= Stack::size());
    // drop also m_stack_types, check type boundaries
    size_t top_bytes = 0;
    auto end_type = m_stack_types.end();
    while (top_bytes < first) {
        end_type --;
        top_bytes += end_type->size();
    }
    assert(top_bytes == first);
    size_t erase_bytes = size;
    auto begin_type = end_type;
    while (erase_bytes > 0) {
        begin_type --;
        auto type_size = begin_type->size();
        assert(erase_bytes >= type_size);
        erase_bytes -= type_size;
    }
    assert(erase_bytes == 0);
    m_stack_types.erase(begin_type, end_type);
    // remove the requested bytes
    memmove(m_stack.get() + m_stack_pointer + size,
            m_stack.get() + m_stack_pointer, first);
    m_stack_pointer += size;
}


std::ostream& operator<<(std::ostream& os, const Stack& v)
{
    using namespace std;

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
    for (const auto& ti : reverse(v.m_stack_types)) {
        check_print_base();

        const auto size = ti.size();
        cout << setw(4) << right << pos;
        cout << setw(4) << right << size;

        auto value = v.get(pos, ti);
        const auto* hs = value->heapslot();
        if (hs) {
            cout << "  heap:" << std::hex << (intptr_t) hs->data() << std::dec
                 << " refs:" << hs->refcount();
            if (*hs)
                cout << "  " << *value << endl;
            else
                cout << endl;
        } else {
            cout << "  " << *value << endl;
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
    m_stack = move(newstack);
    m_stack_pointer += newcap - m_stack_capacity;
    m_stack_capacity = newcap;
    return m_stack_pointer;
}


void Stack::pop_type(const TypeInfo& ti, size_t s)
{
    if (Stack::size() < s)
        throw StackUnderflow{};

    // check type(s) on stack
    if (ti.type() == Type::Tuple) {
        for (const auto& subtype : ti.subtypes()) {
            assert(subtype == m_stack_types.back());
            (void) subtype;
            m_stack_types.pop_back();
        }
    } else {
        // allow casts - only size have to match
        assert(ti.size() == m_stack_types.back().size());
        m_stack_types.pop_back();
    }
}


} // namespace xci::script
