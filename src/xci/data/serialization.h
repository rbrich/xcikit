// serialization.h created on 2019-03-11, part of XCI toolkit
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

#ifndef XCI_DATA_SERIALIZATION_H
#define XCI_DATA_SERIALIZATION_H

#include "Property.h"
#include <iostream>

namespace xci::data {


/// Writes reflected objects to a stream.
/// The format is custom, text-based.
/// Example:
///
///     object:
///         member1: "value"
///         member2: 123
///         subobject:
///             member: "value"
///         list: "item1"
///         list: "item2"
///         list: "item3"
///         obj_list:
///             member: "value1"
///         obj_list:
///             member: "value2"
///     another_object:
///         name: "abc"
///
/// Scalar types:
/// - int, float (123, 1.23)
/// - bool (false/true)
/// - string ("utf8 text")
///
class TextualWriter {
public:
    explicit TextualWriter(std::ostream& os) : m_stream(os) {}

    template <class T>
    void write(const T& o) {
        meta::doForAllMembers<T>([this, &o](const auto& member) {
            this->write(member.getName(), member.get(o));
        });
    }

    template <class T, typename std::enable_if_t<meta::isRegistered<T>(), int> = 0>
    void write(const char* name, const T& o) {
        m_stream << indent() << name << ":" << std::endl;
        ++m_indent;
        write(o);
        --m_indent;
    }

    template <class T, typename std::enable_if_t<meta::isRegistered<typename T::value_type>(), int> = 0>
    void write(const char* name, const T& o) {
        for (auto& item : o) {
            write(name, item);
        }
    }

    template <class T>
    void write(const char* name, Property<T> value) {
        m_stream << indent() << name << ": " << value.get() << "" << std::endl;
    }

    void write(const char* name, const std::string& value) {
        m_stream << indent() << name << ": \"" << value << "\"" << std::endl;
    }

    void write(const char* name, unsigned int value) {
        m_stream << indent() << name << ": " << value << "" << std::endl;
    }

    void write(const char* name, double value) {
        m_stream << indent() << name << ": " << value << "" << std::endl;
    }

    // Enum with metaobject
    template <class T, typename std::enable_if_t<
            std::is_enum<T>::value && metaobject::has_metaobject<T>(), int> = 0>
    void write(const char* name, T value) {
        m_stream << indent() << name << ": "
            << metaobject::get_enum_constant_name<T>(value) << std::endl;
    }

    // Enum without metaobject (just a number)
    template <class T, typename std::enable_if_t<
            std::is_enum<T>::value && !metaobject::has_metaobject<T>(), int> = 0>
    void write(const char* name, T value) {
        m_stream << indent() << name << ": " << int(value) << std::endl;
    }

private:
    std::string indent(int offset = 0) const {
        return std::string((m_indent + offset) * 4, ' ');
    }

private:
    std::ostream& m_stream;
    unsigned int m_indent = 0;
};



} // namespace xci::data

#endif // include guard
