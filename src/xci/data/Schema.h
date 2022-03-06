// Schema.h created on 2022-02-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_DATA_SCHEMA_H
#define XCI_DATA_SCHEMA_H

#include "ArchiveBase.h"
#include <xci/core/rtti.h>
#include <xci/core/string.h>

#include <magic_enum.hpp>

#include <iostream>
#include <vector>
#include <unordered_map>
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
        using ElemT = typename std::pointer_traits<T>::element_type;
        apply(ArchiveField<Schema, ElemT>{a.key, {}, a.name});
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

    // binary data
    template <BlobType T>
    void add(ArchiveField<Schema, T>&& a) {
        add_member(a.key, a.name, "bytes");
    }

    // iterables
    template <ContainerType T>
    void add(ArchiveField<Schema, T>&& a) {
        // break infinite recursion - don't dive in if the value_type is a struct that was already seen
        if (auto it = m_type_to_struct_idx.find(std::type_index(typeid(typename T::value_type)));
            it != m_type_to_struct_idx.end())
        {
            add_member(a.key, a.name, std::string{m_structs[it->second].name});
            return;
        }
        apply(ArchiveField<Schema, typename T::value_type>{a.key, {}, a.name});
    }

    // variant
    template <typename V, std::size_t I = 0>
    void add_variant_members() {
        if constexpr (I < std::variant_size_v<V>) {
            using VAlt = std::variant_alternative_t<I, V>;
            apply(ArchiveField<Schema, VAlt>{I, {}, name_of_type(typeid(VAlt), "").c_str()});
            add_variant_members<V, I + 1>();
        }
    }

    template <VariantType T>
    void add(ArchiveField<Schema, T>&& a) {
        // index of active alternative
        add_member(draw_next_key(a.key), a.name, "uint" + std::to_string(sizeof(size_t) * 8));
        // value of the alternative
        _enter_group(typeid(T), "variant ", draw_next_key(key_auto), a.name);
        add_variant_members<T>();
        _leave_group();
    }

    template <class Archive>
    void serialize(Archive& ar) {
        ar("struct", m_structs);
    }

private:
    static std::string name_of_type(const std::type_info& ti, std::string fallback) {
        std::string name = core::type_name(ti);
        // templated types ->
        if (name.find('<') != std::string::npos)
            return fallback;
        // strip namespaces
        name = core::rsplit(name, "::", 1).back();
        return name;
    }

    template <typename T>
    void enter_group(const ArchiveField<Schema, T>& a) {
        _enter_group(typeid(T), "struct ", a.key, a.name);
    }
    template <typename T>
    void leave_group(const ArchiveField<Schema, T>& kv) {
        _leave_group();
    }

    void _enter_group(const std::type_info& ti, const std::string& prefix,
                      uint8_t key, const char* name)
    {
        auto [it, add] = m_type_to_struct_idx.try_emplace(std::type_index(ti), 0);
        if (add) {
            it->second = m_structs.size();

            auto type_name = name_of_type(ti, name);
            type_name.insert(0, prefix);

            // Make sure the type name is unique
            auto found = std::find_if(m_structs.begin(), m_structs.end(),
                                      [&type_name](const Struct& s) {
                                          return s.name == type_name;
                                      });
            if (found != m_structs.end()) {
                type_name += '_';
                type_name += std::to_string(it->second);
            }

            m_structs.emplace_back(std::move(type_name));
        }

        add_member(key, name, std::string{m_structs[it->second].name});

        m_group_stack.emplace_back();
        m_group_stack.back().buffer.struct_idx = it->second;
    }

    void _leave_group() {
        m_group_stack.pop_back();
    }

    void add_member(uint8_t key, const char* name, std::string&& type);

    struct Member {
        uint8_t key;
        std::string name;
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

        Struct() = default;
        explicit Struct(std::string&& a_name) : name(std::move(a_name)) {}

        template <class Archive>
        void serialize(Archive& ar) {
            ar("name", name);
            ar("member", members);
        }
    };

    std::vector<Struct> m_structs = {Struct{"struct Main"}};
    std::unordered_map<std::type_index, size_t> m_type_to_struct_idx;
};


} // namespace xci::data

#endif // include guard
