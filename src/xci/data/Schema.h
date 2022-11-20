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

namespace detail {

struct SchemaBufferType {
    size_t struct_idx;
};

} // namespace detail


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
class Schema : public ArchiveBase<Schema>, protected ArchiveGroupStack<detail::SchemaBufferType> {
    friend ArchiveBase<Schema>;

public:
    using Writer = std::true_type;
    using SchemaWriter = std::true_type;
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
        apply(ArchiveField<Schema, typename T::value_type>{a.key, {}, a.name});
    }

    // variant
    template <typename V, std::size_t I = 0>
    void add_variant_members() {
        if constexpr (I < std::variant_size_v<V>) {
            using VAlt = std::variant_alternative_t<I, V>;
            if constexpr (!std::is_same_v<VAlt, std::monostate>) {
                apply(ArchiveField<Schema, VAlt>{I, {}, name_of_type(typeid(VAlt), "").c_str()});
            }
            add_variant_members<V, I + 1>();
        }
    }

    template <VariantType T>
    void add(ArchiveField<Schema, T>&& a) {
        // index of active alternative
        add_member(draw_next_key(a.key), a.name, "variant_id");
        // value of the alternative
        _enter_group(typeid(T), "variant ", draw_next_key(key_auto), a.name);
        add_variant_members<T>();
        _leave_group();
    }

    bool enter_union(const char* name, const char* index_name, const std::type_info& ti) {
        auto name_with_index = fmt::format("{}[{}]", name, index_name);
        return _enter_group(ti, "variant ", draw_next_key(), name_with_index.c_str(), name);
    }

    void leave_union() {
        _leave_group();
    }

    template <class Archive>
    void serialize(Archive& ar) {
        ar("struct", m_structs);
    }

    // -------------------------------------------------------------------------

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

        const Member* member_by_key(uint8_t key) const {
            auto found = std::find_if(members.begin(), members.end(),
                    [key](const Member& m) { return m.key == key; });
            if (found == members.end())
                return {};
            return &*found;
        }

        template <class Archive>
        void serialize(Archive& ar) {
            ar("name", name);
            ar("member", members);
        }
    };

    const Struct& struct_main() const { return m_structs[0]; }

    const Struct* struct_by_name(const std::string& name) const {
        auto found = std::find_if(m_structs.begin(), m_structs.end(),
                [&name](const Struct& s) { return s.name == name; });
        if (found == m_structs.end())
            return {};
        return &*found;
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
    bool enter_group(const ArchiveField<Schema, T>& a) {
        return _enter_group(typeid(T), "struct ", a.key, a.name);
    }
    template <typename T>
    void leave_group(const ArchiveField<Schema, T>& kv) {
        _leave_group();
    }

    bool _enter_group(const std::type_info& ti, const std::string& prefix,
                      uint8_t key, const char* name,
                      std::string fallback_type_name = {});

    void _leave_group() {
        m_group_stack.pop_back();
    }

    void add_member(uint8_t key, const char* name, std::string&& type);

    void init_structs();

    std::vector<Struct> m_structs;
    std::unordered_map<std::type_index, size_t> m_type_to_struct_idx;
};


} // namespace xci::data

#endif // include guard
