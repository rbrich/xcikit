// Property.h created on 2019-03-16, part of XCI toolkit
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

#ifndef XCI_DATA_PROPERTY_H
#define XCI_DATA_PROPERTY_H

namespace xci::data {

// Property with getter/setter functions embedded in the member.
// TODO: usage: Property<std::string, get_name, set_name> name;
// TODO: convenient operators: =, *, ->, ==, etc

template <class T>
class Property {
public:
    template<typename... Args>
    Property(Args&&... args) : m_value(std::forward<Args>(args)...) {}

    void set(T value) { m_value = std::move(value); }
    const T& get() const { return m_value; }

private:
    T m_value;
};



} // namespace xci::data

#endif // include guard
