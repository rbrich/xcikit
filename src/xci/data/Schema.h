// Schema.h created on 2022-02-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_DATA_SCHEMA_H
#define XCI_DATA_SCHEMA_H

#include "ArchiveBase.h"

#include <magic_enum.hpp>

#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <typeindex>

namespace xci::data {


/// Collects key-tag-type information for writing a schema file.
/// Schema itself is serializable.
/// Use BinaryWriter or Dumper to output the schema to a file.
///
/// Usage:
///     // Data root;
///     Schema schema;
///     schema(root);
///     // fstream f; ...
///     BinaryWriter writer(f);
///     writer(schema);
class Schema : public ArchiveBase<Schema> {
    friend ArchiveBase<Schema>;
    struct BufferType { size_t struct_idx; };

public:
    using Writer = std::true_type;
    template<typename T> using FieldType = const T&;

    // raw and smart pointers
    template <FancyPointerType T>
    void add(ArchiveField<Schema, T>&& a) {
        if (!a.value) {
            write_key_name(a.key, a.name);
            return;
        }
        using ElemT = typename std::pointer_traits<T>::element_type;
        apply(ArchiveField<Schema, ElemT>{a.key, *a.value, a.name});
    }

    void add(ArchiveField<Schema, bool>&& a) {
        add_member(a.key, a.name, "bool");
    }

    template <typename T>
    requires std::is_integral_v<T> && std::is_unsigned_v<T>
    void add(ArchiveField<Schema, T>&& a) {
        add_member(a.key, a.name, "uint" + std::to_string(sizeof(T) * 8));
    }

    template <typename T>
    requires std::is_integral_v<T> && std::is_signed_v<T>
    void add(ArchiveField<Schema, T>&& a) {
        add_member(a.key, a.name, "int" + std::to_string(sizeof(T) * 8));
    }

    template <typename T>
    requires std::is_floating_point_v<T>
    void add(ArchiveField<Schema, T>&& a) {
        add_member(a.key, a.name, "float" + std::to_string(sizeof(T) * 8));
    }

    template <typename T>
    requires std::is_enum_v<T>
    void add(ArchiveField<Schema, T>&& a) {
        add_member(a.key, a.name, "enum");
        //magic_enum::enum_type_name(a.value);
    }

    void add(ArchiveField<Schema, std::string>&& a) {
        add_member(a.key, a.name, "string");
    }
    void add(ArchiveField<Schema, const char*>&& a) {
        add_member(a.key, a.name, "string");
    }

    template <typename T>
    requires requires { typename T::iterator; }
    void add(ArchiveField<Schema, T>&& a) {
        apply(ArchiveField<Schema, typename T::value_type>{a.key, *std::begin(a.value), a.name});
    }

    template <class Archive>
    void serialize(Archive& ar) {
        ar("struct", m_structs);
    }

private:
    template <typename T>
    void enter_group(const ArchiveField<Schema, T>& a) {
        auto [it, add] = m_type_to_struct_idx.try_emplace(std::type_index(typeid(a.value)), 0);
        if (add) {
            it->second = m_structs.size();
            m_structs.emplace_back(std::string(a.name) + "_" + std::to_string(it->second));
        }

        add_member(a.key, a.name, std::string(a.name) + "_" + std::to_string(it->second));

        m_group_stack.emplace_back();
        m_group_stack.back().buffer.struct_idx = it->second;
    }
    template <typename T>
    void leave_group(const ArchiveField<Schema, T>& kv) {
        m_group_stack.pop_back();
    }

    void add_member(uint8_t key, const char* name, std::string&& type);

    struct Member {
        uint8_t key;
        const char* name;
        std::string type;

        bool operator==(const Member&) const = default;

        template <class Archive>
        void serialize(Archive& ar) {
            XCI_ARCHIVE(ar, key, name, type);
        }
    };

    struct Struct {
        std::string name;
        std::vector<Member> members;

        Struct(std::string&& a_name) : name(std::move(a_name)) {}

        template <class Archive>
        void serialize(Archive& ar) {
            ar("name", name);
            ar("member", members);
        }
    };
    std::vector<Struct> m_structs = {Struct{"root_0"}};
    std::map<std::type_index, size_t> m_type_to_struct_idx;
};


} // namespace xci::data

#endif // include guard
