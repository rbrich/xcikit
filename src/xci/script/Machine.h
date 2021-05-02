// Machine.h created on 2019-05-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_MACHINE_H
#define XCI_SCRIPT_MACHINE_H

#include "Function.h"
#include "Stack.h"
#include <functional>
#include <stack>

namespace xci::script {


/// Virtual machine

class Machine {
public:
    // Run all Invocations in a function or module:
    // - evaluate each invoked value (possibly concurrently)
    // - pass results to cb
    using InvokeCallback = std::function<void (TypedValue&&)>;
    static constexpr auto no_invoke_cb = [](TypedValue&& v){ v.decref(); };
    void call(const Function& function, const InvokeCallback& cb = no_invoke_cb);

    Stack& stack() { return m_stack; }

    // Trace function calls
    using CallTraceCb = std::function<void(const Function& function)>;
    void set_call_enter_cb(CallTraceCb cb) { m_call_enter_cb = std::move(cb); }
    void set_call_exit_cb(CallTraceCb cb) { m_call_exit_cb = std::move(cb); }

    // Trace bytecode instructions
    // inum     instruction number
    // icode    instruction opcode
    using BytecodeTraceCb = std::function<void(const Function& function, Code::const_iterator ipos)>;
    void set_bytecode_trace_cb(BytecodeTraceCb cb) { m_bytecode_trace_cb = std::move(cb); }

private:
    Stack m_stack;

    // Tracing
    CallTraceCb m_call_enter_cb;
    CallTraceCb m_call_exit_cb;
    BytecodeTraceCb m_bytecode_trace_cb;
};


} // namespace xci::script

#endif // include guard
