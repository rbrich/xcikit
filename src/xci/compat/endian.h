// endian.h created on 2018-11-11, part of XCI toolkit
// Copyright 2018 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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

#elif defined(WIN32)

    #include <cstdlib>

    #if BYTE_ORDER == LITTLE_ENDIAN

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

    #elif BYTE_ORDER == BIG_ENDIAN

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

        #error Unsupported BYTE_ORDER

    #endif

#else

    // Linux
    #include <endian.h>

#endif


#endif // include guard
