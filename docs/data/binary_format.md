Generic Binary Format
=====================

This format is writen by `xci::data::BinaryWriter` and read by `xci::data::BinaryReader`.

The serialization is manual, each serializable object must implement `serialize` method,
which dumps all data in the object, recursively.

## Overview

- byte-order is native, saved in header, respected when loading the file
- values are written verbatim (`memcpy` wherever possible)
- values are organized in hierarchical chunks
- each *chunk* has key, type + optional length, value
- some value types require explicit *length* (string, blob, subchunk)
- one of types is *Master*, which starts subgroup of chunks
- key has 4 bits, maximum number of distinct keys is 16
- objects with more fields can be encoded using subgroups
- types are predefined, keys are custom - each chunk can have different keys

File header: `<MAGIC:16><VERSION:8><FLAGS:8>`
- `MAGIC:16`: 0xCB, 0xDF (CBDF = Chunked Binary Data Format)
- `VERSION:8`: 0x30 (ASCII '0')
- `FLAGS:8`: 0x01
    - bits 0,1: endianness: LE=01, BE=10, reserved 00, 11
    - bits 2-7: reserved

General chunk format: `<TYPE:4><KEY:4>[<LEN:varint>][<VALUE>]`
- `TYPE:4`:
    - 0 = Null
    - 1 = Bool: false
    - 2 = Bool: true
    - 3 = Byte, VALUE is 1 byte, unsigned
    - 4 = UInt32, VALUE is 4 bytes, unsigned
    - 5 = UInt64, VALUE is 8 bytes, unsigned
    - 6 = Int32, VALUE is 4 bytes, signed, two's complement format
    - 7 = Int64, VALUE is 8 bytes, signed, two's complement format
    - 8 = Float32, VALUE is 4 bytes, IEEE 754 format
    - 9 = Float64, VALUE is 8 bytes, IEEE 754 format
    - 10 = Varint, RESERVED - not implemented
    - 11 = Array, VALUE contains SUBTYPE + LEN, see below
    - 12 = String, has LEN, contains UTF-8 string (not zero-terminated)
    - 13 = Binary Data, has LEN, contains arbitrary data
    - 14 = Master, starts a group
    - 15 = Terminator, ends a group, KEY must be same as in related Master
- `KEY:4`: custom key identifying the semantic content
    - same key can occur multiple times to encode heterogeneous container
- `LEN`, `VALUE` - depends on type
- Array type:
    - homogeneous array, VALUE = `<SUBTYPE:4><COUNT:varint>[<ITEM>...]`
    - SUBTYPE can be 0-9 (only fixed-sized types)
    - COUNT is number of items in array
    - COUNT starts with 4 bits (to fill first byte with SUBTYPE) and may
      extend to more bytes using varint encoding
- Final terminator:
  - the whole file is terminated with special Terminator byte: `0xFF`
  - `KEY:4`: must be 15 (no special meaning)
  - `TYPE:4`: must be 15 = Terminator

File footer: `<CRC:32>`

Minimal valid file content: `<HEADER:32><TERMINATOR:8><CRC:32>`
- `CB DF 30 01  FF  ?? ?? ?? ??` (9 bytes)
