// BinaryBase.h created on 2019-03-14 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_DATA_BINARY_BASE_H
#define XCI_DATA_BINARY_BASE_H

#include "ArchiveBase.h"
#include "Crc32.h"
#include <cstdint>
#include <vector>
#include <bitset>

namespace xci::data {


class ArchiveBadMagic : public ArchiveError {
public:
    ArchiveBadMagic() : ArchiveError("Bad magic") {}
};


class ArchiveBadVersion : public ArchiveError {
public:
    ArchiveBadVersion() : ArchiveError("Bad version") {}
};


class ArchiveBadFlags: public ArchiveError {
public:
    ArchiveBadFlags() : ArchiveError("Bad flags") {}
};


class ArchiveBadChunkType : public ArchiveError {
public:
    ArchiveBadChunkType() : ArchiveError("Bad chunk type") {}
};

// Reading "const char*" would require malloc, which is probably not what you want.
// Thus, C strings can be only dumped, not read.
// Workaround: Read into std::string temporary and copy into const char* as needed.
class ArchiveCannotReadCString : public ArchiveError {
public:
    ArchiveCannotReadCString() : ArchiveError("Cannot read C string from archive") {}
};

class ArchiveBadChecksum : public ArchiveError {
public:
    ArchiveBadChecksum() : ArchiveError("Bad checksum") {}
};


class ArchiveReadError : public ArchiveError {
public:
    ArchiveReadError() : ArchiveError("Error reading from archive") {}
};


class ArchiveUnexpectedEnd : public ArchiveError {
public:
    ArchiveUnexpectedEnd() : ArchiveError("Corrupted archive (chunk size larger than available data)") {}
};


class ArchiveMissingChecksum : public ArchiveError {
public:
    ArchiveMissingChecksum() : ArchiveError("Archive checksum not found") {}
};


struct BinaryBase {

    enum Header: uint8_t {
        // magic: CBDF (Chunked Binary Data Format)
        Magic0              = 0xCB,
        Magic1              = 0xDF,
        // version: '0' (first version of the format)
        Version             = 0x30,
    };

    enum Endianness: uint8_t {
        LittleEndian    = 0b00000001,
        BigEndian       = 0b00000010,
        EndiannessMask  = 0b00000011,
    };

    enum Checksum: uint8_t {
        ChecksumCNone   = 0b00000000,
        ChecksumCrc32   = 0b00000100,
        ChecksumSha256  = 0b00001000,
        ChecksumMask    = 0b00001100,
    };

    enum Type: uint8_t {
        // chunk type, upper 4 bits
        Null        =  0 << 4,
        BoolFalse   =  1 << 4,
        BoolTrue    =  2 << 4,
        Byte        =  3 << 4,
        UInt32      =  4 << 4,
        UInt64      =  5 << 4,
        Int32       =  6 << 4,
        Int64       =  7 << 4,
        Float32     =  8 << 4,
        Float64     =  9 << 4,
        VarInt      = 10 << 4,
        Array       = 11 << 4,
        String      = 12 << 4,
        Binary      = 13 << 4,
        Master      = 14 << 4,
        Control     = 15 << 4,

        TypeMask    = 0xF0,
        KeyMask     = 0x0F,

        ChunkNotFound = 0xFF,
    };

    template<class T> requires std::is_enum_v<T>
    static constexpr Type to_chunk_type() { return to_chunk_type<std::underlying_type_t<T>>(); }

    template<class T> requires (sizeof(T) == 1) && std::is_integral_v<T>
    static constexpr Type to_chunk_type() { return Type::Byte; }

    template<class T> requires (sizeof(T) == 4) && std::is_integral_v<T> && std::is_unsigned_v<T>
    static constexpr Type to_chunk_type() { return Type::UInt32; }

    template<class T> requires (sizeof(T) == 8) && std::is_integral_v<T> && std::is_unsigned_v<T>
    static constexpr Type to_chunk_type() { return Type::UInt64; }

    template<class T> requires (sizeof(T) == 4) && std::is_integral_v<T> && std::is_signed_v<T>
    static constexpr Type to_chunk_type() { return Type::Int32; }

    template<class T> requires (sizeof(T) == 8) && std::is_integral_v<T> && std::is_signed_v<T>
    static constexpr Type to_chunk_type() { return Type::Int64; }

    template<class T> requires std::is_same_v<T, float>
    static constexpr Type to_chunk_type() { return Type::Float32; }

    template<class T> requires std::is_same_v<T, double>
    static constexpr Type to_chunk_type() { return Type::Float64; }

    static constexpr size_t size_by_type(uint8_t type) {
        switch (type) {
            case Null:
            case BoolFalse:
            case BoolTrue:
                return 0;
            case Byte:
                return 1;
            case UInt32:
            case Int32:
            case Float32:
                return 4;
            case UInt64:
            case Int64:
            case Float64:
                return 8;
            default:
                return size_t(-1);
        }
    }

    static constexpr bool type_has_len(uint8_t type) {
        return type == VarInt || type == Array || type == String || type == Binary || type == Master;
    }

    enum ControlSubtype: uint8_t {
        Metadata    = 0,
        Data        = 1,
    };
};


} // namespace xci::data

#endif // include guard
