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
- chunks are ordered by key, from low to high (only exception is Control chunk,
  whose KEY field has meaning of SUBTYPE)
- objects with more fields can be encoded using Extended chunks
- types are predefined, keys are custom - each chunk can have different keys

File header: `<MAGIC:16><VERSION:8><FLAGS:8><SIZE:var>`
- `MAGIC:16`: 0xCB, 0xDF (CBDF = Chunked Binary Data Format)
- `VERSION:8`: 0x30 (ASCII '0')
- `FLAGS:8`: 0x01
    - bits 0,1: endianness: LE=01, BE=10, reserved 00, 11
    - bits 2,3: checksum: None=00, CRC32=01, SHA256=10, reserved 11
    - bits 4,5: compression: None=00, Deflate=01, reserved 10, 11
    - bits 6,7: reserved
- `SIZE:var`: LEB128 encoded size of file content (does not include header)

General chunk format: `<TYPE:4><KEY/SUBTYPE:4>[<LEN:var>][<VALUE>]`
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
    - 10 = VarInt, has LEN, signed 1-N bytes (this is different from encoding of LEN)
    - 11 = Array, has LEN, VALUE contains SUBTYPE, see below
    - 12 = String, has LEN, contains UTF-8 string (not zero-terminated)
    - 13 = Binary Data, has LEN, contains arbitrary data
    - 14 = Master, has LEN, starts a group
    - 15 = Control, has SUBTYPE instead of KEY, see below
- `KEY:4`: custom key identifying the semantic content
    - same key can occur multiple times to encode heterogeneous container
    - chunks are ordered by KEY
    - Control chunk, TYPE=15 - SUBTYPE:
        - 0 = Metadata (see below)
        - 1 = Data (see below)
        - 2-15 = reserved
- `LEN`: optional, depends on type
    - length by type:
        - 0-2:    0
        - 3:      1
        - 4,6,8:  4
        - 5,7,9:  8
        - 10-14:  LEN field
        - 15:     0
    - encoding: LEB128
- `VALUE` - optional, depends on type
- Array type:
    - homogeneous array, VALUE = `<SUBTYPE:4><RESERVED:4>[<ITEM>...]`
    - SUBTYPE can be 0-9 (only fixed-sized types)
    - number of items in array = LEN / sizeof(SUBTYPE)

Metadata:
- Introduced and optionally terminated by chunk TYPE=15 (Control)
- At root level, metadata are expected at first, data start with 0xF1 (Control/Data)
- Recognized metadata at root level:
    - At beginning - file descriptors:
        - File type, content type...
        - TODO: specify
    - At end - checksums/signatures:
        - Each metadata item is saved in its own chunk, with well-known key and type
        - The VALUE is the checksum itself and it's computed from previous file content
          including previous metadata chunks.
        - The checksum chunk itself and any chunks following it are excluded.
          Checksum's KEY/TYPE byte is included in checksum.
        - CRC32: KEY=1, TYPE=4 (UInt32)
        - SHA256: KEY=1, TYPE=13 (blob), LEN=32

Minimal valid file content: header with SIZE=0, no CRC
- `CB DF 30 01  00` (5 bytes)
