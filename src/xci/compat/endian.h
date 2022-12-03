// endian.h created on 2018-11-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_COMPAT_ENDIAN_H
#define XCI_COMPAT_ENDIAN_H


#ifdef __APPLE__

    #include <libkern/OSByteOrder.h>

    #define htobe16(x) OSSwapHostToBigInt16(x)
    #define htole16(x) OSSwapHostToLittleInt16(x)
    #define be16toh(x) OSSwapBigToHostInt16(x)
    #define le16toh(x) OSSwapLittleToHostInt16(x)

    #define htobe32(x) OSSwapHostToBigInt32(x)
    #define htole32(x) OSSwapHostToLittleInt32(x)
    #define be32toh(x) OSSwapBigToHostInt32(x)
    #define le32toh(x) OSSwapLittleToHostInt32(x)

    #define htobe64(x) OSSwapHostToBigInt64(x)
    #define htole64(x) OSSwapHostToLittleInt64(x)
    #define be64toh(x) OSSwapBigToHostInt64(x)
    #define le64toh(x) OSSwapLittleToHostInt64(x)

#elif defined(_WIN32)

    #include <cstdlib>

    #define LITTLE_ENDIAN 1
    #define BIG_ENDIAN 2

    #if defined(_M_IX86) || defined(_M_X64) || defined(_M_ARM)

        #define BYTE_ORDER LITTLE_ENDIAN

        #define htobe16(x) _byteswap_ushort(x)
        #define htole16(x) (x)
        #define be16toh(x) _byteswap_ushort(x)
        #define le16toh(x) (x)

        #define htobe32(x) _byteswap_ulong(x)
        #define htole32(x) (x)
        #define be32toh(x) _byteswap_ulong(x)
        #define le32toh(x) (x)

        #define htobe64(x) _byteswap_uint64(x)
        #define htole64(x) (x)
        #define be64toh(x) _byteswap_uint64(x)
        #define le64toh(x) (x)

    #elif defined(_M_PPC)

        #define BYTE_ORDER BIG_ENDIAN

        #define htobe16(x) (x)
        #define htole16(x) _byteswap_ushort(x)
        #define be16toh(x) (x)
        #define le16toh(x) _byteswap_ushort(x)

        #define htobe32(x) (x)
        #define htole32(x) _byteswap_ulong(x)
        #define be32toh(x) (x)
        #define le32toh(x) _byteswap_ulong(x)

        #define htobe64(x) (x)
        #define htole64(x) _byteswap_uint64(x)
        #define be64toh(x) (x)
        #define le64toh(x) _byteswap_uint64(x)

    #else

        #error "Unsupported target machine"

    #endif

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)

    #include <sys/types.h>
    #include <sys/endian.h>

#else

    // Linux
    #include <endian.h>

#endif


// Sanity check - order macros are defined:
#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN) || !defined(BIG_ENDIAN)
#error "Endian macros are not available!"
#endif


#endif // include guard
