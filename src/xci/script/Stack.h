// Stack.h created on 2019-05-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_STACK_H
#define XCI_SCRIPT_STACK_H

#include "Value.h"
#include <xci/core/container/ChunkedStack.h>
#include <cstddef>  // byte
#include <vector>
#include <iostream>

namespace xci::script {

class Function;


struct StackTraceFrame {
    std::string function_name;
};

using StackTrace = std::vector<StackTraceFrame>;



/// Call stack
///
/// The main stack is down-growing, with small initial size,
/// but resized when full (up to maximum allowed size).
///
/// Includes two auxiliary stacks:
/// - TypeInfo stack for keeping record of types of data on main stack
///   (this is optional, might be disabled for non-debug programs)
/// - Frame stack for keeping record of called functions and return addresses
///
/// Also keeps track of current set of I/O streams. Enter/leave functions
/// modify the current streams, push the original stream on main stack
/// and restore it from there.

class Stack {
public:
    Stack() = default;
    explicit Stack(size_t init_capacity) : m_stack_capacity(init_capacity) {}

    using StackAbs = size_t;  // address into stack, zero is the bottom (this is basically negative address, bad for reasoning, but it's stable when stack grows)
    using StackRel = size_t;  // address into stack, zero is stack pointer (top of the stack, address grows to the bottom)
    using CodeOffs = size_t;  // instruction pointer (bytecode offset from beginning of the function)

    StackRel to_rel(StackAbs abs) const { return size() - abs; }
    StackAbs to_abs(StackRel rel) const { return size() - rel; }

    void push(const Value& v);
    void push(const TypedValue& v) { push(v.value()); }

    Value pull(const TypeInfo& type_info);
    TypedValue pull_typed(const TypeInfo& type_info) { return TypedValue(pull(type_info), type_info); }

    template <ValueT T>
    T pull() {
        // create empty value
        T v;
        pop_type(v);
        // read value from stack
        m_stack_pointer += v.read(data());
        return v;
    }

    Value get(StackRel pos, const TypeInfo& ti) const;
    Value get(StackRel pos, Type type) const;  // cannot be used for Tuple
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

    // Swap two values on top of the stack.
    // Top `first` bytes are swapped with `second` bytes below them.
    void swap(size_t first, size_t second);

    bool empty() const { return m_stack_capacity == m_stack_pointer; }
    StackAbs size() const { return m_stack_capacity - m_stack_pointer; }
    size_t capacity() const { return m_stack_capacity; }

    // Get moving pointer to top of the stack (lowest valid address)
    // The address changes with each operation.
    std::byte* data() const { return &m_stack[m_stack_pointer]; }

    // ------------------------------------------------------------------------
    // Type tracking

    size_t n_values() const { return m_stack_types.size(); }
    Type top_type() const { return m_stack_types.back(); }

    // ------------------------------------------------------------------------
    // Function return addresses

    struct Frame {
        const Function& function;
        CodeOffs instruction;  // return address
        StackAbs base;  // parameters are below base, local variables above
        explicit Frame(const Function& fun, CodeOffs ins, size_t base)
                : function(fun), instruction(ins), base(base) {}
    };

    // top frame points to currently running function
    void push_frame(const Function& fun) { m_frame.emplace(fun, 0, size()); }
    void pop_frame() { m_frame.pop(); }
    Frame& frame() { return m_frame.top(); }
    const Frame& frame() const { return m_frame.top(); }
    const Frame& frame(size_t pos) const { return m_frame[pos]; }
    size_t n_frames() const { return m_frame.size(); }

    // ------------------------------------------------------------------------
    // Unwinding

    StackTrace make_trace();

    // ------------------------------------------------------------------------
    // I/O Streams
    // Here we store the initial streams. As the program runs, these values
    // are swapped to stack and back. Effectively, the values here are
    // "top of the stack" and the previous values are stored on the value stack.

    struct Streams {
        value::Stream in { Stream::default_stdin() };
        value::Stream out { Stream::default_stdout() };
        value::Stream err { Stream::default_stderr() };

        ~Streams() {
            in.decref();
            out.decref();
            err.decref();
        }
    };

    script::Stream stream_in() const { return m_streams.in.value(); }
    script::Stream stream_out() const { return m_streams.out.value(); }
    script::Stream stream_err() const { return m_streams.err.value(); }

    void swap_stream_in(value::Stream& in) { std::swap(m_streams.in, in); }
    void swap_stream_out(value::Stream& out) { std::swap(m_streams.out, out); }
    void swap_stream_err(value::Stream& err) { std::swap(m_streams.err, err); }

    const value::Stream& get_stream_in() { m_streams.in.incref(); return m_streams.in; }
    const value::Stream& get_stream_out() { m_streams.out.incref(); return m_streams.out; }
    const value::Stream& get_stream_err() { m_streams.err.incref(); return m_streams.err; }

    // ------------------------------------------------------------------------

    friend std::ostream& operator<<(std::ostream& os, const Stack& v);

    // reserve more space for stack, returning new free space
    size_t grow();

private:
    void push_type(const Value& v);

    // throw if the stack would underflow
    // or when the top isn't compatible with the type
    void pop_type(const Value& v);

    static constexpr size_t m_stack_max = 100*1024*1024;
    size_t m_stack_capacity = 1024;
    size_t m_stack_pointer = m_stack_capacity;
    std::unique_ptr<std::byte[]> m_stack = std::make_unique<std::byte[]>(m_stack_capacity);
    std::vector<Type> m_stack_types;
    core::ChunkedStack<Frame> m_frame;
    Streams m_streams;
};


} // namespace xci::script

#endif // include guard
