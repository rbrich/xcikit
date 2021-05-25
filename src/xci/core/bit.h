// bit.h created on 2018-11-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)
//
// ------------------------------------------------------------------
// Bitwise operations - <bit> header compatibility + goodies
//
// References:
// - https://en.cppreference.com/w/cpp/header/bit
// - http://graphics.stanford.edu/~seander/bithacks.html
// ------------------------------------------------------------------

#ifndef XCI_COMPAT_BIT_H
#define XCI_COMPAT_BIT_H

#include <type_traits>
#include <cstring>
#include <cstdint>

namespace xci {


/// Similar to C++20 bit_cast, but copy bits from void*, char*, byte* etc.
/// Does not check type sizes. Useful to emulate file reading from memory buffer.
///
/// Example:
///
///     vector<byte> buf;
///     auto ptr = buf.data();
///     auto a = bit_copy<int32_t>(ptr);
///     auto b = bit_copy<uint16_t>(ptr + 4);

template <class To, class From>
typename std::enable_if<
        std::is_trivially_copyable<From>::value &&
        std::is_trivial<To>::value &&
        !std::is_pointer<From>::value &&
        sizeof(From) == 1,
        To>::type
bit_copy(const From* src) noexcept
{
    To dst;
    std::memcpy(&dst, src, sizeof(To));
    return dst;
}


/// Read bits from iterator pointing to std::byte or other 1-byte type.
/// Advance the iterator by number of bytes read.
///
/// Example:
///
///     vector<byte> buf;
///     auto ptr = buf.data();
///     auto a = bit_read<int32_t>(ptr);
///     auto b = bit_read<uint16_t>(ptr);

template <typename OutT, typename InIter>
requires requires (InIter iter, size_t s) {
    std::is_trivial_v<OutT>;
    sizeof(*iter) == 1;
    iter += s;
}
OutT bit_read(InIter& iter) noexcept
{
    OutT dst;
    std::memcpy(&dst, &*iter, sizeof(OutT));
    iter += sizeof(OutT);
    return dst;
}


} // namespace

#endif // include guard
