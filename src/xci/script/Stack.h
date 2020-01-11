// Stack.h created on 2019-05-18, part of XCI toolkit
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

#ifndef XCI_SCRIPT_STACK_H
#define XCI_SCRIPT_STACK_H

#include "Value.h"
#include "Function.h"
#include "Error.h"
#include <xci/core/Stack.h>
#include <xci/compat/utility.h>
#include <xci/compat/bit.h>
#include <vector>

namespace xci::script {


/// Call stack
///
/// The main stack is down-growing, with small initial size,
/// but resized when full (up to maximum allowed size).
///
/// Includes two auxiliary stacks:
/// - TypeInfo stack for keeping record of types of data on main stack
///   (this is optional, might be disabled for non-debug programs)
/// - Frame stack for keeping record of called functions and return addresses

class Stack {
public:
    Stack() = default;
    explicit Stack(size_t init_capacity) : m_stack_capacity(init_capacity) {}

    using StackAbs = size_t;  // address into stack, zero is the bottom (this is basically negative address, bad for reasoning, but it's stable when stack grows)
    using StackRel = size_t;  // address into stack, zero is stack pointer (top of the stack, address grows to the bottom)

    StackRel to_rel(StackAbs abs) const { return size() - abs; }
    StackAbs to_abs(StackRel rel) const { return size() - rel; }

    void push(const Value& o);

    std::unique_ptr<Value> pull(const TypeInfo& type_info);

    template <typename T,
              typename = std::enable_if_t<std::is_base_of<Value, T>::value>>
    T pull() {
        // create empty value
        T v;
        auto s = v.size();
        if (size() < s)
            throw StackUnderflow{};
        // read value from stack
        v.read(&m_stack[m_stack_pointer]);
        m_stack_pointer += s;
        // check type on stack (allow casts - only size have to match)
        assert(v.type_info().size() == m_stack_types.back().size());
        m_stack_types.pop_back();
        return v;
    }

    std::unique_ptr<Value> get(StackRel pos, const TypeInfo& ti) const;
    void* get_ptr(StackRel pos) const;
    void clear_ptr(StackRel pos);

    // Copy `size` bytes from `addr` to top of the stack
    void copy(StackRel pos, size_t size);

    // Remove bytes in range `first`..`first + size` from top of the stack.
    // Exactly `size` bytes is removed.
    // E.g.:
    //      drop(0, 4) removes top 4 bytes
    //      drop(4, 2) leaves top 4 bytes and removes following 2
    //      drop(4, 0) is no-op
    void drop(StackRel first, size_t size);

    bool empty() const { return m_stack_capacity == m_stack_pointer; }
    StackAbs size() const { return m_stack_capacity - m_stack_pointer; }
    size_t capacity() const { return m_stack_capacity; }

    // Get moving pointer to top of the stack (lowest valid address)
    // The address changes with each operation.
    byte* data() const { return &m_stack[m_stack_pointer]; }

    // ------------------------------------------------------------------------
    // Type tracking

    size_t n_values() const { return m_stack_types.size(); }

    // ------------------------------------------------------------------------

    struct Frame {
        const Function* function;
        Index instruction;
        StackAbs base;  // parameters are below base, local variables above
        explicit Frame(const Function* fun, Index ins, size_t base)
            : function(fun), instruction(ins), base(base) {}
    };

    void push_frame(const Function* fun, Index ins) { m_frame.emplace(fun, ins, size()); }
    void pop_frame() { m_frame.pop(); }
    const Frame& frame() const { return m_frame.top(); }
    const Frame& frame(size_t pos) const { return m_frame[pos]; }
    size_t n_frames() const { return m_frame.size(); }


    friend std::ostream& operator<<(std::ostream& os, const Stack& v);

    // reserve more space for stack, returning new free space
    size_t grow();

private:
    static constexpr size_t m_stack_max = 100*1024*1024;
    size_t m_stack_capacity = 1024;
    size_t m_stack_pointer = m_stack_capacity;
    std::unique_ptr<byte[]> m_stack = std::make_unique<byte[]>(m_stack_capacity);
    std::vector<TypeInfo> m_stack_types;
    core::Stack<Frame> m_frame;
};


} // namespace xci::script

#endif // include guard
