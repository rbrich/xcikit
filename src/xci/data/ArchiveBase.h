// ArchiveBase.h created on 2020-06-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_DATA_ARCHIVE_BASE_H
#define XCI_DATA_ARCHIVE_BASE_H

#include <xci/core/error.h>
#include <xci/core/macros/foreach.h>
#include <boost/pfr/precise/core.hpp>
#include <vector>
#include <cstdint>

#ifndef __cpp_concepts
#error "xci::data require C++20 concepts"
#endif

namespace xci::data {


template <typename T>
struct ArchiveField {
    uint8_t key = 255;
    T& value;
    const char* name = nullptr;
};
template<typename T> ArchiveField(uint8_t, T&) -> ArchiveField<T>;
template<typename T> ArchiveField(uint8_t, T&, const char*) -> ArchiveField<T>;

// Automatic named fields
#define XCI_ARCHIVE_FIELD(i, mbr) xci::data::ArchiveField{i, mbr, #mbr}
#define XCI_ARCHIVE(ar, ...)                                                                        \
    ar(                                                                                             \
        XCI_FOREACH(XCI_ARCHIVE_FIELD, 255, XCI_COMMA, __VA_ARGS__)                                 \
    )


template<typename T, typename TArchive>
concept TypeWithSerialize = requires(T& v, TArchive& ar) { v.serialize(ar); };

template<typename T, typename TArchive>
concept TypeWithArchiveSupport = requires(T& v, std::uint8_t k, TArchive& ar) { ar.add(ArchiveField<T>{k, v}); };

template<typename T>
concept FancyPointerType = requires(const T& v) { *v; typename std::pointer_traits<T>::pointer; };


class ArchiveError : public core::Error {
public:
    explicit ArchiveError(std::string&& msg) : core::Error(std::move(msg)) {}
};


class ArchiveOutOfKeys : public ArchiveError {
public:
    ArchiveOutOfKeys() : ArchiveError("Tried to allocate more than 16 keys per object.") {}
};


class ArchiveKeyNotInOrder : public ArchiveError {
public:
    ArchiveKeyNotInOrder() : ArchiveError("Requested key cannot be allocated (not in order).") {}
};


template <class TImpl>
class ArchiveBase {
public:
    ArchiveBase() { m_group_stack.emplace_back(); }

    // convenience: ar(value, ...) -> ar.apply(value), ...
    template<typename ...Args>
    void operator() (Args&&... args) {
        ((void) apply(std::forward<Args>(args)), ...);
    }

    // convenience: ar.apply(value) -> ar.apply(ArchiveField{key_auto, value})
    template <typename T>
    void apply(T& value) {
        apply(ArchiveField<T>{key_auto, value});
    }

    // when: the type has serialize() method
    template <TypeWithSerialize<TImpl> T>
    void apply(ArchiveField<T>&& kv) {
        kv.key = draw_next_key(kv.key);
        static_cast<TImpl*>(this)->enter_group(kv.key, kv.name);
        kv.value.serialize(*static_cast<TImpl*>(this));
        static_cast<TImpl*>(this)->leave_group(kv.key, kv.name);
    }

    // when: Archive implementation has add() method for the type
    template <TypeWithArchiveSupport<TImpl> T>
    void apply(ArchiveField<T>&& kv) {
        kv.key = draw_next_key(kv.key);
        static_cast<TImpl*>(this)->add(std::forward<ArchiveField<T>>(kv));
    }

    // when: other non-polymorphic structs - use pfr
    template <typename T>
    requires (std::is_class_v<T> && !std::is_polymorphic_v<T> &&
            !TypeWithSerialize<T, TImpl> && !TypeWithArchiveSupport<T, TImpl>)
    void apply(ArchiveField<T>&& kv) {
        kv.key = draw_next_key(kv.key);
        static_cast<TImpl*>(this)->enter_group(kv.key, kv.name);
        boost::pfr::for_each_field(kv.value, [&](auto& field) {
            apply(field);
        });
        static_cast<TImpl*>(this)->leave_group(kv.key, kv.name);
    }

protected:

    static constexpr uint8_t key_max = 15;
    static constexpr uint8_t key_auto = 255;
    static constexpr uint8_t key_same = 128;
    constexpr uint8_t reuse_same_key(uint8_t key) { return key + key_same; }
    uint8_t draw_next_key(uint8_t req = key_auto) {
        auto& group = m_group_stack.back();

        // for arrays - do not draw new key, reuse the old one
        if (req >= key_same && req <= key_same + key_max) {
            req -= key_same;
            if (req != group.next_key - 1)
                throw ArchiveKeyNotInOrder();
            return req;
        }

        // specific key requested?
        if (req != key_auto) {
            if (req < group.next_key)
                throw ArchiveKeyNotInOrder();
            // respect the request, possibly skipping some keys
            group.next_key = req;
        }

        if (group.next_key > key_max)
            throw ArchiveOutOfKeys();

        return group.next_key ++;
    }

    bool is_root_group() const { return m_group_stack.size() == 1; }
    auto& group_buffer() { return m_group_stack.back().buffer; }

    // ===== data =====

    struct Group {
        uint8_t next_key = 0;
        typename TImpl::BufferType buffer {};
    };
    std::vector<Group> m_group_stack;
};


} // namespace xci::data

#endif // include guard
