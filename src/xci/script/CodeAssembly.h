// CodeAssembly.h created on 2023-08-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_CODE_ASSEMBLY_H
#define XCI_SCRIPT_CODE_ASSEMBLY_H

#include "Code.h"

namespace xci::script {


/// Higher-level bytecode representation.
/// All instructions are relocatable, jump offsets are generated during assembly to Code.
class CodeAssembly
{
public:
    enum class Annotation: size_t {
        Label = 1000,                          // arg2 = index of label, removed during assembly
        Jump = size_t(Opcode::Jump),           // arg2 = index of label, replaced by Opcode::Jump
        JumpIfNot = size_t(Opcode::JumpIfNot), // arg2 = index of label, replaced by Opcode::JumpIfNot
    };

    struct Instruction {
        Opcode opcode = Opcode::Noop;
        std::pair<size_t, size_t> args;

        uint8_t arg_B1() const { return static_cast<uint8_t>(args.first); }
    };

    /// Translate to binary representation and append to Code
    void assemble_to(Code& code);

    /// Translate from binary representation.
    /// Decodes the LEB128 encoded args and replaces JUMP instructions
    /// by Jump+Label annotations, which are relocatable.
    /// UNCHECKED: Does not check for premature end of code - will crash!
    ///     To safely disassemble code from untrusted source,
    ///     you need to add (at least) two zero bytes to the end as padding.
    ///     These will be decoded as NOOP and can be removed afterwards.
    ///     In case of trailing L2 instruction, those bytes
    ///     would be consumed as the instruction args.
    void disassemble(const Code& code);

    using const_iterator = std::vector<Instruction>::const_iterator;
    const_iterator begin() const { return m_instr.begin(); }
    const_iterator end() const { return m_instr.end(); }
    size_t size() const { return m_instr.size(); }
    bool empty() const { return m_instr.empty(); }
    const Instruction& operator[](size_t i) const { return m_instr[i]; }
    Instruction& operator[](size_t i) { return m_instr[i]; }

    Instruction& back() noexcept { return m_instr.back(); }

private:
    struct Label {
        size_t addr;  // disassembly: address where the label will appear
                      // assembly: base address for computing jump offset
        bool processed = false;
    };
    void disassemble_labels(size_t addr, std::vector<Label>& labels);
    void assemble_repeat_jumps(Code& code, std::vector<Label>& labels);

    std::vector<Instruction> m_instr;
    size_t m_labels = 0;  // number of jump targets
};


} // namespace xci::script

#endif  // include guard
