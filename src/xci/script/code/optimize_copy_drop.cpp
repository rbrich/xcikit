// optimize_copy_drop.cpp created on 2023-08-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "optimize_copy_drop.h"
#include <xci/script/Module.h>
#include <xci/script/CodeAssembly.h>
#include <xci/script/code/assembly_helpers.h>

#include <range/v3/view/enumerate.hpp>

namespace xci::script {

using ranges::views::enumerate;


static void move_drop_up(Function& fn, CodeAssembly& ca, size_t i)
{
    auto* drop = &ca[i];
    while (i > 0) {
        --i;
        auto& prev = ca[i];
        switch (prev.opcode) {
            case Opcode::Call0:
            case Opcode::Call1:
            case Opcode::Call: {
                const auto& f = get_call_function(prev, fn.module());
                const auto f_ret = f.signature().return_type.size();
                auto& drop_skip = drop->args.first;
                if (drop_skip >= f_ret) {
                    drop_skip += f.raw_size_of_parameter() - f_ret;
                    std::swap(*drop, prev);
                    drop = &prev;
                    continue;  // the loop
                }
                break;
            }
            default:
                break;
        }
        break;  // the loop
    }
}


namespace {
static size_t i_offset(const CodeAssembly::Instruction& instr) { return instr.args.first; }
static size_t i_size(const CodeAssembly::Instruction& instr) { return instr.args.second; }
}

static void merge_contiguous_copies(Function& fn, CodeAssembly& ca, size_t i)
{
    auto& copy1 = ca[i++];
    while (i < ca.size()) {
        const auto& copy2 = ca[i];
        // Is the following instruction a continuous copy?
        if (copy2.opcode != Opcode::Copy || i_offset(copy2) + i_size(copy2) != i_offset(copy1))
            break;
        copy1.args = {i_offset(copy2), i_size(copy1) + i_size(copy2)};
        ca.remove(i);
    }
}


static void eliminate_copy_drop(Function& fn, CodeAssembly& ca, size_t i)
{
    auto& copy = ca[i];
    auto& drop = ca[i+1];
    if (drop.opcode == Opcode::Drop
    && i_size(copy) == i_size(drop)
    && i_offset(copy) == 0 && i_offset(drop) == i_size(copy)) {
        ca.remove(i, 2);
    }
}


void optimize_copy_drop(Function& fn)
{
    CodeAssembly& ca = fn.asm_code();

    for (const auto& [i, instr] : ca | enumerate) {
        if (instr.opcode == Opcode::Drop) {
            // Move DROP instruction up before any CALLs
            move_drop_up(fn, ca, i);
        }
    }

    // NOTE: ca.size() may change during the loop
    for (size_t i = 0; i < ca.size(); ++i) {
        if (ca[i].opcode == Opcode::Copy) {
            // Merge multiple COPY instructions that in effect copy one continuous block of bytes
            merge_contiguous_copies(fn, ca, i);
            // Then check if following DROP is affecting all copied bytes and eliminate both COPY/DROP
            eliminate_copy_drop(fn, ca, i);
        }
    }
}


} // namespace xci::script
