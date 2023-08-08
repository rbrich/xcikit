// assembly_helpers.cpp created on 2023-08-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "assembly_helpers.h"
#include <xci/script/Function.h>
#include <xci/script/Module.h>

namespace xci::script {


const Function& get_call_function(const CodeAssembly::Instruction& instr, const Module& mod)
{
    const Module* module;
    Module::FunctionIdx fn_idx;
    if (instr.opcode == Opcode::Call0) {
        module = &mod;
        fn_idx = instr.args.first;
    } else if (instr.opcode == Opcode::Call1) {
        module = &mod.get_imported_module(0);
        fn_idx = instr.args.first;
    } else if (instr.opcode == Opcode::Call) {
        module = &mod.get_imported_module(instr.args.first);
        fn_idx = instr.args.second;
    } else {
        assert(!"not a call instruction");
        const Function* r = nullptr;
        return *r;
    }
    return module->get_function(fn_idx);
}


} // namespace xci::script
