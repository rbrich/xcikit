// ArchiveBase.h created on 2020-06-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_DATA_ARCHIVE_BASE_H
#define XCI_DATA_ARCHIVE_BASE_H

#include <xci/core/error.h>
#include <xci/core/macros/foreach.h>
#include <boost/pfr/core.hpp>
#include <vector>
#include <cstdint>

#ifndef __cpp_concepts
#error "xci::data require C++20 concepts"
#endif

namespace xci::data {


template <typename Archive, typename T>
struct ArchiveField {
    uint8_t key = 255;
    typename Archive::template FieldType<T> value;
    const char* name = nullptr;
};

// Automatic named fields
#define XCI_ARCHIVE_FIELD(i, mbr) xci::data::ArchiveField<Archive, decltype(mbr)>{i, mbr, #mbr}
#define XCI_ARCHIVE(ar, ...)                                                                        \
    ar(                                                                                             \
        XCI_FOREACH(XCI_ARCHIVE_FIELD, 255, XCI_COMMA, __VA_ARGS__)                                 \
    )


// in-class method: void serialize(Archive&)
template<typename T, typename TArchive>
concept TypeWithSerializeMethod = requires(T& v, TArchive& ar) { v.serialize(ar); };

// out-of-class function: void serialize(Archive&, T&)
template<typename T, typename TArchive>
concept TypeWithSerializeFunction = requires(T& v, TArchive& ar) { serialize(ar, v); };

// in-class method: void save(Archive&) const
template<typename T, typename TArchive>
concept TypeWithSaveMethod = requires(const T& v, TArchive& ar) { typename TArchive::Writer; v.save(ar); };

// out-of-class function: void save(Archive&, const T&)
template<typename T, typename TArchive>
concept TypeWithSaveFunction = requires(const T& v, TArchive& ar) { typename TArchive::Writer; save(ar, v); };

// in-class method: void load(Archive&)
template<typename T, typename TArchive>
concept TypeWithLoadMethod = requires(T& v, TArchive& ar) { typename TArchive::Reader; v.load(ar); };

// out-of-class function: void load(Archive&, T&)
template<typename T, typename TArchive>
concept TypeWithLoadFunction = requires(T& v, TArchive& ar) { typename TArchive::Reader; load(ar, v); };

template<typename T, typename TArchive>
concept ArchiveIsReader = requires(T& v, std::uint8_t k, TArchive& ar) { typename TArchive::Reader; ArchiveField<TArchive, T>{k, v}; };

template<typename T, typename TArchive>
concept ArchiveIsWriter = requires(const T& v, std::uint8_t k, TArchive& ar) { typename TArchive::Writer; ArchiveField<TArchive, T>{k, v}; };

template<typename T, typename TArchive>
concept TypeWithReaderSupport = requires(T& v, std::uint8_t k, TArchive& ar) { typename TArchive::Reader; ar.add(ArchiveField<TArchive, T>{k, v}); };

template<typename T, typename TArchive>
concept TypeWithWriterSupport = requires(const T& v, std::uint8_t k, TArchive& ar) { typename TArchive::Writer; ar.add(ArchiveField<TArchive, T>{k, v}); };

template<typename T, typename TArchive>
concept TypeWithMagicSupport =
        !TypeWithSerializeMethod<T, TArchive> &&
        !TypeWithSerializeFunction<T, TArchive> &&
        !TypeWithSaveMethod<T, TArchive> &&
        !TypeWithSaveFunction<T, TArchive> &&
        !TypeWithLoadMethod<T, TArchive> &&
        !TypeWithLoadFunction<T, TArchive> &&
        !TypeWithReaderSupport<T, TArchive> &&
        !TypeWithWriterSupport<T, TArchive> &&
        std::is_class_v<T> &&
        !std::is_polymorphic_v<T> &&
        std::is_copy_constructible_v<T>;

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
    template <ArchiveIsReader<TImpl> T>
    void apply(T& value) {
        apply(ArchiveField<TImpl, T>{key_auto, value});
    }
    template <ArchiveIsWriter<TImpl> T>
    void apply(const T& value) {
        apply(ArchiveField<TImpl, T>{key_auto, value});
    }

    // convenience: ar.kv(key, value) -> ar.apply(ArchiveField{key, value})
    template <ArchiveIsReader<TImpl> T>
    void kv(uint8_t key, T& value) {
        apply(ArchiveField<TImpl, T>{key, value});
    }
    template <ArchiveIsWriter<TImpl> T>
    void kv(uint8_t key, const T& value) {
        apply(ArchiveField<TImpl, T>{key, value});
    }

    // when: the type has serialize() method
    template <TypeWithSerializeMethod<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = draw_next_key(kv.key);
        static_cast<TImpl*>(this)->enter_group(kv.key, kv.name);
        const_cast<T&>(kv.value).serialize(*static_cast<TImpl*>(this));
        static_cast<TImpl*>(this)->leave_group(kv.key, kv.name);
    }

    // when: the type has out-of-class serialize() function
    template <TypeWithSerializeFunction<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = draw_next_key(kv.key);
        static_cast<TImpl*>(this)->enter_group(kv.key, kv.name);
        serialize(*static_cast<TImpl*>(this), const_cast<T&>(kv.value));
        static_cast<TImpl*>(this)->leave_group(kv.key, kv.name);
    }

    // when: the type has save() method and this is Writer
    template <TypeWithSaveMethod<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = draw_next_key(kv.key);
        static_cast<TImpl*>(this)->enter_group(kv.key, kv.name);
        kv.value.save(*static_cast<TImpl*>(this));
        static_cast<TImpl*>(this)->leave_group(kv.key, kv.name);
    }

    // when: the type has out-of-class save() function and this is Writer
    template <TypeWithSaveFunction<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = draw_next_key(kv.key);
        static_cast<TImpl*>(this)->enter_group(kv.key, kv.name);
        save(*static_cast<TImpl*>(this), kv.value);
        static_cast<TImpl*>(this)->leave_group(kv.key, kv.name);
    }

    // when: the type has load() method and this is Reader
    template <TypeWithLoadMethod<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = draw_next_key(kv.key);
        static_cast<TImpl*>(this)->enter_group(kv.key, kv.name);
        kv.value.load(*static_cast<TImpl*>(this));
        static_cast<TImpl*>(this)->leave_group(kv.key, kv.name);
    }

    // when: the type has out-of-class load() function and this is Reader
    template <TypeWithLoadFunction<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = draw_next_key(kv.key);
        static_cast<TImpl*>(this)->enter_group(kv.key, kv.name);
        load(*static_cast<TImpl*>(this), kv.value);
        static_cast<TImpl*>(this)->leave_group(kv.key, kv.name);
    }

    // when: Archive implementation has add() method for the type
    template <TypeWithReaderSupport<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = draw_next_key(kv.key);
        static_cast<TImpl*>(this)->add(std::forward<ArchiveField<TImpl, T>>(kv));
    }
    template <TypeWithWriterSupport<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = draw_next_key(kv.key);
        static_cast<TImpl*>(this)->add(std::forward<ArchiveField<TImpl, T>>(kv));
    }

    // when: other non-polymorphic structs - use pfr
    template <TypeWithMagicSupport<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
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
    uint8_t draw_next_key(uint8_t req = key_auto) {
        auto& group = m_group_stack.back();

        // specific key requested?
        if (req != key_auto) {
            // request for same key as before - don't increment
            if (req == group.next_key - 1)
                return req;
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
