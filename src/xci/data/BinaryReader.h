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
#include <xci/data/reflection.h>
#include <xci/compat/endian.h>

#include <istream>
#include <map>

namespace xci::data {


class BinaryReader : private BinaryBase {
public:
    explicit BinaryReader(std::istream& is) : m_stream(is) {}

    template <class T>
    void load(T& o) {
        read_header();
        read(o);
        read_footer();
    }

    void read_header();
    void read_footer();

    template <class T, typename std::enable_if_t<meta::isRegistered<T>(), int> = 0>
    void read(T& o) {
        uint8_t type = 0;
        uint64_t len = 0;
        for (;;) {
            read_type_len(type, len);

            // Process end of member object (this is special, it has no key)
            if (type == Type_Master && len == Master_Leave) {
                --m_depth;
                break;
            }

            // CRC32 at depth=0 ends the file
            if (type == Type_Checksum && len == sizeof(m_crc) && m_depth == 0)
                break;

            auto* key = read_key();
            switch (type) {
                case Type_Master:
                    // enter member object (read a sub-object)
                    if (len == Master_Enter) {
                        ++m_depth;
                        meta::doForAllMembers<T>(
                            [this, key, &o](const auto& member) {
                                if (!strcmp(key, member.getName())) {
                                    this->read(member.getRef(o));
                                }
                            });
                    } else {
                        m_stream.setstate(std::ios::failbit);
                        m_error = Error::BadFieldType;
                        return;
                    }
                    break;
                case Type_Float:
                case Type_Integer:
                    meta::doForAllMembers<T>(
                            [this, key, &o](const auto& member) {
                                if (!strcmp(key, member.getName())) {
                                    this->read(member.getRef(o));
                                }
                            });
                    break;
                case Type_String: {
                    std::string value(len, 0);
                    read(value);
                    meta::setMemberValue<std::string>(o, key, std::move(value));
                    break;
                }
                default:
                    m_stream.setstate(std::ios::failbit);
                    m_error = Error::BadFieldType;
                    return;
            }
        }
    }

    void read(std::string& value);

    template <class T, typename std::enable_if_t<meta::isRegistered<typename T::value_type>(), int> = 0>
    void read(T& out) {
        typename T::value_type value;
        read(value);
        out.push_back(value);
    }

    template <class T, typename std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value, int> = 0>
    void read(T& out) {
        unsigned int value = 0;
        read_with_crc(value);
        out = T(le32toh(value));
    }

    template <class T, typename std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
    void read(T& out) {
        double value = 0.;
        read_with_crc(value);
        out = T(value);
    }

    enum class Error {
        None,
        BadMagic,
        BadVersion,
        BadFlags,
        BadFieldType,
        BadChecksum,
    };

    Error get_error() const { return m_error; }
    size_t get_last_pos() const { return m_pos; }
    const char* get_error_cstr() const {
        switch (m_error) {
            case Error::None: return "None";
            case Error::BadMagic: return "Bad magic";
            case Error::BadVersion: return "Bad version";
            case Error::BadFlags: return "Bad flags";
            case Error::BadFieldType: return "Bad field type";
            case Error::BadChecksum: return "Bad checksum";
        }
        __builtin_unreachable();
    }

private:
    template <typename T>
    void read_with_crc(T& value) {
        read_with_crc((uint8_t*)&value, sizeof(value));
    }
    void read_with_crc(uint8_t* buffer, size_t length);

    void read_type_len(uint8_t& type, uint64_t& len);
    const char* read_key();

private:
    std::istream& m_stream;
    Error m_error = Error::None;
    std::map<size_t, std::string> m_pos_to_key;
    size_t m_pos = 0;
    int m_depth = 0;
};


} // namespace xci::data

#endif // include guard
