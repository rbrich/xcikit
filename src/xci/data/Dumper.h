// Dumper.h created on 2019-03-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_DATA_DUMPER_H
#define XCI_DATA_DUMPER_H

#include "ArchiveBase.h"

#include <magic_enum.hpp>

#include <iostream>
#include <vector>
#include <memory>

namespace xci::data {


/// Writes serializable objects to a stream.
///
/// The format is custom, text-based. Example:
///
///     (0) object:
///         (0) member1: "value"
///         (1) member2: 123
///         (2) subobject:
///             (0) member: "value"
///         (3) list: "item1"
///         (3) list: "item2"
///         (3) list: "item3"
///         (4) obj_list:
///             (0) member: "value1"
///         (4) obj_list:
///             (0) member: "value2"
///     (1) another_object:
///         (0) name: "abc"
///
/// Scalar types:
/// - int, float (123, 1.23)
/// - bool (false/true)
/// - string ("utf8 text")
///
class Dumper : public ArchiveBase<Dumper> {
    friend ArchiveBase<Dumper>;
    struct BufferType {};

public:
    using Writer = std::true_type;
    template<typename T> using FieldType = const T&;

    explicit Dumper(std::ostream& os) : m_stream(os) {}

    // raw and smart pointers
    template <FancyPointerType T>
    void add(ArchiveField<Dumper, T>&& a) {
        if (!a.value) {
            write_key_name(a.key, a.name);
            return;
        }
        using ElemT = typename std::pointer_traits<T>::element_type;
        apply(ArchiveField<Dumper, ElemT>{a.key, *a.value, a.name});
    }

    void add(ArchiveField<Dumper, bool>&& a) {
        write_key_name(a.key, a.name);
        m_stream << std::boolalpha << a.value << std::endl;
    }

    template <typename T>
    requires std::is_same_v<T, uint8_t>
    void add(ArchiveField<Dumper, T>&& a) {
        write_key_name(a.key, a.name);
        m_stream << unsigned(a.value) << std::endl;
    }

    // integers, floats
    template <typename T>
    requires (std::is_integral_v<T> && !std::is_same_v<T, uint8_t>) || std::is_floating_point_v<T>
    void add(ArchiveField<Dumper, T>&& a) {
        write_key_name(a.key, a.name);
        m_stream << a.value << std::endl;
    }

    template <typename T>
    requires std::is_enum_v<T>
    void add(ArchiveField<Dumper, T>&& a) {
        write_key_name(a.key, a.name);
        m_stream << magic_enum::enum_name(a.value) << std::endl;
    }

    // string
    void add(ArchiveField<Dumper, std::string>&& a) {
        write_key_name(a.key, a.name);
        m_stream << '"' << a.value << '"' << std::endl;
    }
    void add(ArchiveField<Dumper, const char*>&& a) {
        write_key_name(a.key, a.name);
        m_stream << '"' << (a.value? a.value : "<null>") << '"' << std::endl;
    }

    template <typename T>
    requires requires { typename T::iterator; }
    void add(ArchiveField<Dumper, T>&& a) {
        for (auto& item : a.value) {
            apply(ArchiveField<Dumper, typename T::value_type>{a.key, item, a.name});
        }
    }

private:
    template <typename T>
    void enter_group(const ArchiveField<Dumper, T>& kv) {
        write_key_name(kv.key, kv.name, '\n');
        m_group_stack.emplace_back();
    }
    template <typename T>
    void leave_group(const ArchiveField<Dumper, T>& kv) {
        m_group_stack.pop_back();
    }

    std::string indent(int offset = 0) const {
        return std::string((m_group_stack.size() - 1 + offset) * 4, ' ');
    }

    void write_key_name(uint8_t key, const char* name, char sep = ' ');

    std::ostream& m_stream;
};


} // namespace xci::data

#endif // include guard
