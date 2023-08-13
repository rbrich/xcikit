// assembly_helpers.h created on 2023-08-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)


#ifndef XCI_SCRIPT_CODE_ASSEMBLY_HELPERS_H
#define XCI_SCRIPT_CODE_ASSEMBLY_HELPERS_H

#include <xci/script/CodeAssembly.h>

namespace xci::script {

class Function;
class Module;


const Function& get_call_function(const CodeAssembly::Instruction& instr, const Module& mod);


} // namespace xci::script

#endif  // include guard
