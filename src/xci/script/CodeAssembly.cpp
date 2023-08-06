// CodeAssembly.cpp created on 2023-08-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "CodeAssembly.h"
#include <xci/data/coding/leb128.h>
#include <xci/core/log.h>

#include <range/v3/view/enumerate.hpp>

namespace xci::script {

using namespace xci::core;
using xci::data::leb128_decode;
using ranges::views::enumerate;


void CodeAssembly::disassemble_labels(size_t addr, std::vector<Label>& labels)
{
    for (const auto& [label_idx, label] : labels | enumerate) {
        if (label.processed)
            continue;
        if (label.addr < addr) {
            log::error("CodeAssembly::disassemble: invalid jump target, not at instruction boundary: {}", label.addr);
            label.processed = true;
            continue;
        }
        if (label.addr == addr) {
            Instruction& instr = m_instr.emplace_back();
            instr.opcode = Opcode::Annotation;
            instr.args.first = size_t(Annotation::Label);
            instr.args.second = label_idx;
            label.processed = true;
            continue;
        }
        // otherwise the label addr is bigger - continue
    }
}


void CodeAssembly::disassemble(const Code& code)
{
    std::vector<Label> labels;
    auto it = code.begin();
    while (it != code.end()) {
        Instruction& instr = m_instr.emplace_back();
        instr.opcode = static_cast<Opcode>(*it); ++it;
        if (instr.opcode >= Opcode::B1ArgFirst && instr.opcode <= Opcode::B1ArgLast) {
            instr.args.first = *it; ++it;
            if (instr.opcode == Opcode::Jump || instr.opcode == Opcode::JumpIfNot) {
                size_t addr = it - code.begin();
                labels.push_back({.addr = addr + instr.args.first});
                // replace Jump instructions by annotations
                instr.args.first = size_t(instr.opcode);
                instr.args.second = labels.size() - 1;
                instr.opcode = Opcode::Annotation;
            }
        } else
        if (instr.opcode >= Opcode::L1ArgFirst && instr.opcode <= Opcode::L1ArgLast) {
            instr.args.first = leb128_decode<size_t>(it);
        } else
        if (instr.opcode >= Opcode::L2ArgFirst && instr.opcode <= Opcode::L2ArgLast) {
            instr.args = std::make_pair(leb128_decode<size_t>(it), leb128_decode<size_t>(it));
        }
        // else: opcode has no args
        disassemble_labels(it - code.begin(), labels);
    }
    m_labels = labels.size();
}


// The JUMP instructions has byte argument, max jump offset is +255 bytes.
// Check for unprocessed jumps that are most far away and repeat them,
// i.e. generate a new jump and write jump address to original jump.
void CodeAssembly::assemble_repeat_jumps(Code& code, std::vector<Label>& labels)
{
    for (auto& label : labels) {
        if (label.processed)
            continue;
        if (code.size() - label.addr > 255 - 21 - 2) {
            // 21 is the longest instruction code possible (L2 with two max size_t args)
            // 2 is size of JUMP instruction with B1 arg
            code.add_B1(Opcode::Jump, 2);  // jump over the repeated jump
            code.set_arg_B(label.addr - 1, code.size());  // update original jump to here
            code.add_B1(Opcode::Jump, 0);  // generate repeated jump
            label.addr = code.size();  // update label to point to the repeated jump
        }
    }
}


void CodeAssembly::assemble_to(Code& code)
{
    std::vector<Label> labels;
    labels.resize(m_labels);
    for (const Instruction& instr : m_instr) {
        if (instr.opcode == Opcode::Annotation) {
            const auto annot = Annotation(instr.args.first);
            const auto label_idx = instr.args.second;
            if (annot == Annotation::Jump || annot == Annotation::JumpIfNot) {
                code.add_B1(Opcode(annot), 0);
                labels[label_idx].addr = code.size();
            } else if (annot == Annotation::Label) {
                auto& label = labels[label_idx];
                unsigned jump = code.size() - label.addr;
                assert(jump <= 255);
                code.set_arg_B(label.addr - 1, jump);
                label.processed = true;
            }
            // else: ignore, do not generate any code
        } else
        if (instr.opcode >= Opcode::B1ArgFirst && instr.opcode <= Opcode::B1ArgLast) {
            code.add_B1(instr.opcode, instr.arg_B1());
        } else
        if (instr.opcode >= Opcode::L1ArgFirst && instr.opcode <= Opcode::L1ArgLast) {
            code.add_L1(instr.opcode, instr.args.first);
        } else
        if (instr.opcode >= Opcode::L2ArgFirst && instr.opcode <= Opcode::L2ArgLast) {
            code.add_L2(instr.opcode, instr.args.first, instr.args.second);
        } else {
            code.add_opcode(instr.opcode);
        }
        assemble_repeat_jumps(code, labels);
    }
}


} // namespace xci::script
