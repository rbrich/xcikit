// BinaryReader.h created on 2019-03-14, part of XCI toolkit
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

#ifndef XCI_DATA_BINARY_READER_H
#define XCI_DATA_BINARY_READER_H

#include "BinaryBase.h"
#include <istream>
#include <iostream>
#include <map>

namespace xci::data {


class BinaryReader : private BinaryBase {
public:
    explicit BinaryReader(std::istream& is) : m_stream(is) {}

    template <class T>
    void load(T& o) {
        read_header();
        read(o);
        //read_footer();
    }

    void read_header();
    void read_footer();

    template <class T>
    void read(T& o) {
        uint8_t type = 0;
        uint64_t len = 0;
        read_type_len(type, len);
        auto* key = read_key();

        std::cout << "t:"<< std::hex << int(type) << " l:" << len
        << " k:" << key << std::endl;
    }

    enum class Error {
        None,
        BadMagic,
        BadVersion,
        BadFlags,
    };

    Error get_error() const { return m_error; }
    size_t get_last_pos() const { return m_pos; }
    const char* get_error_cstr() const {
        switch (m_error) {
            case Error::None: return "None";
            case Error::BadMagic: return "Bad magic";
            case Error::BadVersion: return "Bad version";
            case Error::BadFlags: return "Bad flags";
        }
        __builtin_unreachable();
    }

private:
    template <typename T>
    void read_with_crc(T& value) {
        read_with_crc((uint8_t*)&value, sizeof(value));
    }
    void read_with_crc(uint8_t* buffer, size_t length) {
        m_stream.read((char*)buffer, length);
        m_crc = (uint32_t) crc32(m_crc, buffer, (uInt)length);
        m_pos += length;
    }

    void read_type_len(uint8_t& type, uint64_t& len);
    const char* read_key();

private:
    std::istream& m_stream;
    Error m_error = Error::None;
    std::map<size_t, std::string> m_pos_to_key;
    size_t m_pos = 0;
};


} // namespace xci::data

#endif // include guard
