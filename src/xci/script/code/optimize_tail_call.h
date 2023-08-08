// optimize_tail_call.h created on 2023-08-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_CODE_OPTIMIZE_TAIL_CALL_H
#define XCI_SCRIPT_CODE_OPTIMIZE_TAIL_CALL_H

#include <xci/script/Function.h>

namespace xci::script {


/// Replace tail CALL by TAIL_CALL
/// This requires that the CALL is last instruction in the function.
/// TAIL_CALL doesn't add a new stack frame, but replaces the last one.

void optimize_tail_call(Function& fn);


} // namespace xci::script

#endif // include guard
