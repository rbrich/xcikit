// leb128.h created on 2020-06-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_DATA_CODING_LEB128_H
#define XCI_DATA_CODING_LEB128_H

#include <iterator>
#include <limits>
#include <cassert>

/// Implements [LEB128](https://en.wikipedia.org/wiki/LEB128) encoding
/// with additional feature of "skip bits", allowing usage of just a part of first byte.

namespace xci::data {


/// Compute length of value when encoded as LEB128
template <typename InT>
requires std::is_integral_v<InT> && std::is_unsigned_v<InT>
unsigned leb128_length(InT value)
{
    unsigned res = 0;
    do {
        value >>= 7;
        ++res;
    } while (value != 0);
    return res;
}


/// Encode unsigned integer as LEB128 and write it to output iterator.
/// \param iter         Output iterator. Element should be byte/char. Must support ++, * operations.
/// \param value        Integral value to be written.
template <typename InT, typename OutIter,
          typename OutT = typename std::iterator_traits<OutIter>::value_type>
requires requires (OutIter iter, OutT out) {
    std::is_integral_v<InT> && std::is_unsigned_v<InT>;
    *iter = out;
    ++iter;
}
void leb128_encode(OutIter& iter, InT value)
{
    do {
        uint8_t b = value & InT{0x7f};
        value >>= 7;
        if (value != 0)
            b |= 0x80;
        *iter = OutT(b);
        ++iter;
    } while (value != 0);
}


/// Encode unsigned integer as LEB128 and write it to a container with push_back() method.
/// \param iter         Output iterator. Element should be byte/char. Must support ++, * operations.
/// \param value        Integral value to be written.
template <typename InT, typename Container,
          typename OutT = typename Container::value_type>
requires requires (Container& cont, OutT out) {
    std::is_integral_v<InT> && std::is_unsigned_v<InT>;
    cont.push_back(out);
}
void leb128_encode(Container& cont, InT value)
{
    do {
        uint8_t b = value & InT{0x7f};
        value >>= 7;
        if (value != 0)
            b |= 0x80;
        cont.push_back(OutT(b));
    } while (value != 0);
}


/// Decode LEB128 from input iterator (of bytes) to unsigned integer.
/// The decoder stops as soon as overflow is detected.
/// Following bytes are not read even if they still have set the continuation bit.
/// \param iter         Input iterator. Must support ++ (pre-increment), * (get).
/// \return             The decoded integer, or OutT::max if the input won't fit
template <typename OutT, typename InIter>
requires std::is_integral_v<OutT> && std::is_unsigned_v<OutT>
OutT leb128_decode(InIter& iter)
{
    OutT result = uint8_t(*iter) & 0x7f;
    unsigned shift = 0;
    while (uint8_t(*iter) > 0x7f) {
        ++iter;
        shift += 7;
        const uint8_t in_bits = uint8_t(*iter) & 0x7f;
        if (shift + 7 > sizeof(OutT) * 8) {
            // possible overflow
            const unsigned bits_fit = sizeof(OutT) * 8 - shift;
            const uint8_t mask = 0xFF << bits_fit;
            if ((in_bits & mask) != 0) {
                // overflow confirmed
                ++iter;
                return std::numeric_limits<OutT>::max();
            }
        }
        result |= OutT(in_bits) << shift;
    }
    ++iter;
    return result;
}


/// Encode unsigned integer as LEB128 and write it to output iterator.
/// \param iter         Output iterator. Must support ++, * operations.
/// \param value        Integral value to be written.
/// \param skip_bits    N high-order bits of first output byte to be left untouched.
template <typename InT, typename OutIter,
          typename OutT = typename std::iterator_traits<OutIter>::value_type>
requires std::is_integral_v<InT> && std::is_unsigned_v<InT>
void leb128_encode(OutIter& iter, InT value, unsigned skip_bits)
{
    assert(skip_bits < 7);
    // encode first bits to part of first byte
    // NOTE: original content of the non-skipped bits must be cleared,
    //       no zeroing is done here
    *iter |= OutT(value & InT(0x7f >> skip_bits));
    value >>= 7 - skip_bits;
    if (value == 0) {
        // fit in first byte, we're done
        ++iter;
        return;
    }
    *iter |= OutT(0x80 >> skip_bits);
    // middle bytes
    while (value >= 0x80) {
        *++iter = OutT(0x80) | OutT(value & 0x7f);
        value >>= 7;
    }
    // last byte
    *++iter = OutT(value);
    ++iter;
}


/// Decode LEB128 from input iterator (of bytes) to unsigned integer.
/// \param iter         Input iterator. Must support ++ (pre-increment), * (get).
/// \param skip_bits    N high-order bits of first input byte to be skipped.
/// \return             The decoded integer, or OutT::max if the input won't fit
template <typename OutT, typename InIter,
          typename InT = typename std::iterator_traits<InIter>::value_type>
requires std::is_integral_v<OutT> && std::is_unsigned_v<OutT>
OutT leb128_decode(InIter& iter, unsigned skip_bits)
{
    assert(skip_bits < 7);
    OutT result;
    // first byte with skip_bits
    {
        const auto b = *iter;
        ++iter;
        const auto mask_cont = InT(0x80 >> skip_bits);
        const auto mask_data = InT(0x7f >> skip_bits);
        result = OutT(b & mask_data);
        if ((b & mask_cont) != mask_cont)
            return result;
    }
    // other bytes
    unsigned shift = 7 - skip_bits;
    for (;;) {
        const auto b = uint8_t(*iter);
        ++iter;
        const uint8_t in_bits = b & 0x7f;
        if (shift + 7 > sizeof(OutT) * 8) {
            // possible overflow
            const unsigned bits_fit = sizeof(OutT) * 8 - shift;
            const uint8_t mask = 0xFF << bits_fit;
            if ((in_bits & mask) != 0)
                // overflow confirmed
                return std::numeric_limits<OutT>::max();
        }
        result |= OutT(in_bits) << shift;
        if (b < 0x80)
            break;
        shift += 7;
    }
    return result;
}


} // namespace xci::data

#endif // include guard
