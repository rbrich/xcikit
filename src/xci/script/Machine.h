// Machine.h created on 2019-05-18, part of XCI toolkit
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
    using InvokeCallback = std::function<void (const Value&)>;
    void call(const Function& function, const InvokeCallback& cb);

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
