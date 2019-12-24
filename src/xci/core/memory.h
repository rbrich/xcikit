// memory.h created on 2019-12-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_MEMORY_H
#define XCI_CORE_MEMORY_H

namespace xci::core {


/// Take an address or memory size and enlarge it
/// to satisfy alignment requirement. The operation is basically
/// rounding up the address to make it integrally divisible by alignment.
///
/// E.g. align_to(1000, 16) == 1008
///
/// \tparam T           numeric type of the address / size
/// \param address      the address to be rounded up to alignment
/// \param alignment    the alignment requirement

template <class T>
T align_to(T address, T alignment)
{
    return (address + alignment - T{1}) / alignment * alignment;
}


} // namespace xci::core

#endif // include guard
