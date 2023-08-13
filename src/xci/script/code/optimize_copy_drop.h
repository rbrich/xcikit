// optimize_copy_drop.h created on 2023-08-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_CODE_OPTIMIZE_COPY_DROP_H
#define XCI_SCRIPT_CODE_OPTIMIZE_COPY_DROP_H

#include <xci/script/Function.h>

namespace xci::script {


/// Eliminate redundant COPY/DROP pairs

// The common pattern of generated instruction:
//    COPY                0 4
//    COPY                4 4
//    COPY                0 4
//    CALL                1 60 (add (Int32, Int32) -> Int32)
//    CALL                1 60 (add (Int32, Int32) -> Int32)
//    DROP                4 8
//
// This can be rewritten to allow elimination of the COPY/DROP pairs and tail-call optimization:
//    COPY                0 4
//    COPY                4 4
//    COPY                0 4
//    DROP                12 8
//    CALL                1 60 (add (Int32, Int32) -> Int32)
//    CALL                1 60 (add (Int32, Int32) -> Int32)
//
// When moving the DROP up, it must account for CALL parameter/return value:
// * DROP skip must be >= size of return value
// * add the parameter size, subtract the return size (i.e. reverse the change of data stack after calling the function)
// * do not cross any labels (jump targets)
//
// After moving DROP up, the COPY instructions that are immediately followed by DROP can be further optimized.
//
// For example, this copies two 32bit args in same order:
//    COPY                4 4
//    COPY                0 4
//    DROP                8 8
//
// In the first step, it can be optimized to a single COPY:
//    COPY                0 8
//    DROP                8 8
//
// Now it's clearly visible that both instructions are redundant:
// The COPY duplicates top 8 bytes on the stack, the DROP then removes the original bytes.
//
// If the order was being changed, we can translate to SWAP:
//    COPY                0 4
//    COPY                4 4
//    DROP                8 8
//
// Eliminate COPY-DROP by translating to SWAP:
//    SWAP                4 4

void optimize_copy_drop(Function& fn);


} // namespace xci::script

#endif // include guard
