// optimize_tail_call.cpp created on 2023-08-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "optimize_tail_call.h"
#include <xci/script/Module.h>
#include <xci/script/code/assembly_helpers.h>
#include <xci/compat/macros.h>

namespace xci::script {


static Opcode call_to_tail_call(Opcode opcode)
{
    switch (opcode) {
        case Opcode::Call:   return Opcode::TailCall;
        case Opcode::Call0:  return Opcode::TailCall0;
        case Opcode::Call1:  return Opcode::TailCall1;
        default:             XCI_UNREACHABLE;
    }
}


void optimize_tail_call(Function& fn)
{
    CodeAssembly& ca = fn.asm_code();
    if (ca.size() < 2 || ca.back().opcode != Opcode::Ret)
        return;
    auto& prev = ca[ca.size()-2];
    switch (prev.opcode) {
        case Opcode::Call:
        case Opcode::Call0:
        case Opcode::Call1:
            // cannot use tail call for native functions
            if (!get_call_function(prev, fn.module()).is_native()) {
                prev.opcode = call_to_tail_call(prev.opcode);
                ca.pop_back();
            }
            break;
        default:
            break;
    }
}


} // namespace xci::script
