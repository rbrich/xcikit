// Code.h created on 2019-05-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_CODE_H
#define XCI_SCRIPT_CODE_H

#include <fmt/ostream.h>

#include <vector>
#include <ostream>
#include <cstdint>
#include <cstdlib>
#include <cassert>

namespace xci::script {


enum class Opcode: uint8_t {
    // --------------------------------------------------------------
    // A0 (no operands)

    Noop,

    LogicalNot,
    LogicalOr,
    LogicalAnd,

    BitwiseNot_8,
    BitwiseNot_32,
    BitwiseNot_64,
    BitwiseOr_8,
    BitwiseOr_32,
    BitwiseOr_64,
    BitwiseAnd_8,
    BitwiseAnd_32,
    BitwiseAnd_64,
    BitwiseXor_8,
    BitwiseXor_32,
    BitwiseXor_64,

    // Control flow
    Execute,                // pull closure from stack, unwrap it, call the contained function

    // --------------------------------------------------------------
    // B1 (one single-byte operand)

    // Cast Int/Float value to another type
    // Operand: 4/4 bit split, high half = from type, low half = to type
    // Casting rules are based on the C++ implementation (static_cast).
    //
    // Type numbers:
    // * unsigned integers: 1 = 8bit, (2 = 16bit), 3 = 32bit, 4 = 64bit, (5 = 128bit)
    // * signed integers: (6 = 8bit, 7 = 16bit), 8 = 32bit, 9 = 64bit, (A = 128bit)
    // * floats: (B = 16bit), C = 32bit, D = 64bit, (E = 128bit)
    Cast,

    // Comparison instructions
    // Operand: 4/4 bit split, high half = left-hand type, low half = right-hand type
    // Only pairs of same types are defined, operations on distinct types are reserved.
    // Type numbers are the same as for cast instruction above.

    Equal,
    NotEqual,
    LessEqual,
    GreaterEqual,
    LessThan,
    GreaterThan,

    // Arithmetic instructions
    //
    // Operand: 4/4 bit split, high half = left-hand type, low half = right-hand type
    // Only pairs of same types are defined, operations on distinct types are reserved
    // for possible future optimization. (The machine would coerce types by itself,
    // but the caller would need to know the result of coercion.)
    // Type numbers are the same as for cast instruction above.

    Neg,
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Exp,

    // Bitwise shift
    // Operand: 4/4 bit split, high half = left-hand type, low half = right-hand type
    // Only pairs of same types are defined, operations on distinct types are reserved.
    // Defined only for integer types. ShiftLeft is same for signed/unsigned types.
    // ShiftRight does sign extension for signed types.
    // Type numbers are the same as for cast instruction above.
    ShiftLeft,
    ShiftRight,

    Jump,                   // operand = relative jump (+N instructions) - unconditional
    JumpIfNot,              // pull a bool from stack, operand = relative jump (+N instructions) if cond is false

    // --------------------------------------------------------------
    // L1 (one LEB128-encoded operand)

    LoadStatic,             // operand = idx of static in module, push on stack
    LoadModule,             // operand = idx of imported module, push it as value on stack
    LoadFunction,           // operand = idx of function in module, push on stack

    Call0,                  // operand = idx of function in current module, call it, pull args from stack, push result back
    Call1,                  // operand = idx of function in builtin module, call it, pull args from stack, push result back

    MakeClosure,            // operand = idx of function in current module, pull nonlocals from stack, wrap them into closure, push closure back
    SetBase,                // set base for Copy etc., operand = number of stack frames to climb up

    IncRef,                 // operand = offset from top, (uint32*) at the offset is dereferenced and incremented
    DecRef,                 // operand = offset from top, (uint32*) at the offset is dereferenced and decremented

    ListSubscript,          // operand = elem type (type index), get list element - pull the list, index:Int32 from stack, push element
    ListLength,             // operand = elem type (type index), get list length - pull the list, push length:UInt32
    ListSlice,              // operand = elem type, slice a list - pull the list, pull begin:Int, end:Int, step:Int, push sliced list
    ListConcat,             // operand = elem type, concat two lists - pull a, b lists from stack, push a + b

    Invoke,                 // operand = type index in current module, pull value from stack, invoke it

    // --------------------------------------------------------------
    // L2 (two LEB128-encoded operands)

    Call,                   // operand1 = idx of imported module, operand2 = idx of function in the module, call it, pull args from stack, push result back

    MakeList,               // operand1 = number of elements, operand2 = elem type (type index), pulls number elems from stack, creates list on heap, pushes list handle back to stack

    Copy,                   // operand1 = offset from base (0 = base = first arg), copy <operand2> bytes from stack and push them back on top
    Drop,                   // remove a value from stack: skip top <operand1> bytes, then remove <operand2> bytes
    Swap,                   // swap values on stack: <operand1> bytes from top with following <operand2> bytes

    Annotation,             // used only in CodeAssembly, must not appear in Code

    // --------------------------------------------------------------
    // Auxiliary aliases

    A0First = Noop,
    A0Last = Execute,
    B1First = Cast,
    B1Last = JumpIfNot,
    L1First = LoadStatic,
    L1Last = Invoke,
    L2First = Call,
    L2Last = Annotation,
};

// Allow basic arithmetic on OpCode
inline Opcode operator+(Opcode lhs, int rhs) { return Opcode(int(lhs) + rhs); }

std::ostream& operator<<(std::ostream& os, Opcode v);


class Code {
public:
    using OpIdx = size_t;

    // convenience
    void add_opcode(Opcode opcode) {
        add(static_cast<uint8_t>(opcode));
    }
    // opcodes with operands
    // Note: These can't be overloads, the semantics are different.
    void add_B1(Opcode opcode, uint8_t operand) {
        add_opcode(opcode);
        add(operand);
    }
    size_t add_L1(Opcode opcode, size_t operand);
    size_t add_L2(Opcode opcode, size_t operand1, size_t operand2);

    void add(uint8_t b) { m_ops.push_back(b); }
    void set(OpIdx pos, uint8_t b) { m_ops[pos] = b; }
    OpIdx this_instruction_address() const { return m_ops.size() - 1; }

    using const_iterator = std::vector<uint8_t>::const_iterator;
    const_iterator begin() const { return m_ops.begin(); }
    const_iterator end() const { return m_ops.end(); }
    size_t size() const { return m_ops.size(); }
    bool empty() const { return m_ops.empty(); }

    bool operator==(const Code& rhs) const { return m_ops == rhs.m_ops; }

    template<class Archive>
    void serialize(Archive& ar) {
        ar("ops", m_ops);
    }

private:
    std::vector<uint8_t> m_ops;
};



} // namespace xci::script

template <> struct fmt::formatter<xci::script::Opcode> : ostream_formatter {};

#endif // include guard
