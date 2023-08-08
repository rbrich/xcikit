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


static void move_tail_drop_up(Function& fn, CodeAssembly& ca, size_t i)
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


void optimize_copy_drop(Function& fn)
{
    CodeAssembly& ca = fn.asm_code();
    for (const auto& [idx, instr] : ca | enumerate) {
        if (instr.opcode == Opcode::Drop) {
            move_tail_drop_up(fn, ca, idx);
        }
    }

    // TODO: eliminate COPY/DROP pairs
}


} // namespace xci::script
