// BinaryWriter.h created on 2019-03-13, part of XCI toolkit
// Copyright 2019 Radek Brich
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

#ifndef XCI_DATA_BINARY_WRITER_H
#define XCI_DATA_BINARY_WRITER_H

#include "BinaryBase.h"
#include <xci/data/reflection.h>
#include <xci/compat/endian.h>
#include <xci/compat/bit.h>
#include <ostream>
#include <map>

namespace xci::data {


/// Writes reflected objects to a binary stream.
/// The format is custom:
/// - byte-order is configurable and saved in header
/// - keys are compressed to 8..16 bits
/// - maximum number of distinct keys is 32768
/// - header is: <MAGIC:16><VERSION:8><FLAGS:8>
/// - the general format is: <TYPE:3><FLAG:1><LEN:4>[<+LEN>]<KEY><VALUE>
/// - key is encoded:
///   - first appearance - length + string: <FLAG:1><LEN:7><CHARS> (flag=0)
///   - keys are truncated to 127 chars (max LEN:7)
///   - next appearance, offset from first app. less than 32768: <FLAG:1><OFS:15> (flag=1)
///   - remember pos before reading offset, subtract offset, seek, read <LEN:7><CHARS> (flag is zero an can be ignored)
/// - footer is: <TYPE_FLAG_LEN:8><CRC:32> (type=7,flag=0,len=4)
/// - master chunks contain sub-chunks, terminated with master chunk with LEN=1
/// See source code for actual values.

class BinaryWriter : private BinaryBase {
public:
    explicit BinaryWriter(std::ostream& os) : m_stream(os) {}

    template <class T>
    void dump(const T& o) {
        write_header();
        write(o);
        write_footer();
    }

    void write_header();
    void write_footer();

    template <class T>
    void write(const T& o) {
        meta::doForAllMembers<T>([this, &o](const auto& member) {
            this->write(member.getName(), member.get(o));
        });
    }

    template <class T, typename std::enable_if_t<meta::isRegistered<T>(), int> = 0>
    void write(const char* name, const T& o) {
        write_master(Master_Enter);
        write_key(name);
        write(o);
        write_master(Master_Leave);
    }

    template <class T, typename std::enable_if_t<meta::isRegistered<typename T::value_type>(), int> = 0>
    void write(const char* name, const T& o) {
        for (auto& item : o) {
            write(name, item);
        }
    }

    template <class T, typename std::enable_if_t<std::is_enum<T>::value, int> = 0>
    void write(const char* name, T value) {
        write(name, (unsigned int) value);
    }

    void write(const char* name, const std::string& value);
    void write(const char* name, unsigned int value);
    void write(const char* name, double value);

private:
    template <typename T>
    void write_with_crc(const T& value) {
        write_with_crc((uint8_t*)&value, sizeof(value));
    }
    void write_with_crc(const uint8_t* buffer, size_t length) {
        m_stream.write((char*)buffer, length);
        m_crc = (uint32_t) crc32(m_crc, buffer, (uInt)length);
        m_pos += length;
    }

    void write_type_len(uint8_t type, uint64_t len);
    void write_key(const char* key);
    void write_master(int flag);


private:
    std::ostream& m_stream;
    std::map<std::string, size_t> m_key_to_pos;
    size_t m_pos = 0;
    int m_depth = 0;
};


} // namespace xci::data

#endif // include guard
