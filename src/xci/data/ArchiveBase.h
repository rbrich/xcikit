// ArchiveBase.h created on 2020-06-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_DATA_ARCHIVE_BASE_H
#define XCI_DATA_ARCHIVE_BASE_H

#include <xci/core/error.h>
#include <xci/core/macros/foreach.h>

#include <fmt/core.h>
#ifndef XCI_ARCHIVE_NO_MAGIC
#include <boost/pfr/core.hpp>
#endif

#include <vector>
#include <variant>
#include <cstdint>

#ifndef __cpp_concepts
#error "xci::data require C++20 concepts"
#endif

#include <concepts>

namespace xci::data {


template <typename Archive, typename T>
struct ArchiveField {
    struct archive_field_tag {};

    uint8_t key = 255;
    typename Archive::template FieldType<T> value;
    const char* name = nullptr;
};

// Automatic named fields
#define XCI_ARCHIVE_FIELD_AUTOKEY(ar, mbr) ar(255, #mbr, mbr)
#define XCI_ARCHIVE_FIELD(ar, i, mbr) ar(i, #mbr, mbr)
#define XCI_ARCHIVE(ar, ...)                                                                        \
        XCI_FOREACH(XCI_ARCHIVE_FIELD_AUTOKEY, ar, XCI_SEMICOLON, __VA_ARGS__);


// in-class method: void serialize(Archive&)
template<typename T, typename TArchive>
concept TypeWithSerializeMethod = requires(T& v, TArchive& ar) { v.serialize(ar); };

// out-of-class function: void serialize(Archive&, T&)
template<typename T, typename TArchive>
concept TypeWithSerializeFunction = requires(T& v, TArchive& ar) { serialize(ar, v); };

// in-class method: void save_schema(Archive&) const
template<typename T, typename TArchive>
concept TypeWithSchemaMethod = requires(const T& v, TArchive& ar) { typename TArchive::SchemaWriter; v.save_schema(ar); };

// out-of-class function: void save_schema(Archive&, const T&)
template<typename T, typename TArchive>
concept TypeWithSchemaFunction = requires(const T& v, TArchive& ar) { typename TArchive::SchemaWriter; save_schema(ar, v); };

// in-class method: void save(Archive&) const
template<typename T, typename TArchive>
concept TypeWithSaveMethod = !TypeWithSchemaMethod<T, TArchive> &&
    requires(const T& v, TArchive& ar) { typename TArchive::Writer; v.save(ar); };

// out-of-class function: void save(Archive&, const T&)
template<typename T, typename TArchive>
concept TypeWithSaveFunction = !TypeWithSchemaFunction<T, TArchive> &&
    requires(const T& v, TArchive& ar) { typename TArchive::Writer; save(ar, v); };

// in-class method: void load(Archive&)
template<typename T, typename TArchive>
concept TypeWithLoadMethod = requires(T& v, TArchive& ar) { typename TArchive::Reader; v.load(ar); };

// out-of-class function: void load(Archive&, T&)
template<typename T, typename TArchive>
concept TypeWithLoadFunction = requires(T& v, TArchive& ar) { typename TArchive::Reader; load(ar, v); };

template<typename T, typename TArchive>
concept ArchiveIsReader = requires(T& v, std::uint8_t k, TArchive& ar) { typename TArchive::Reader; ArchiveField<TArchive, T>{k, v}; };

template<typename T, typename TArchive>
concept ArchiveIsWriter = !requires { typename T::archive_field_tag; } && requires(const T& v, std::uint8_t k, TArchive& ar) { typename TArchive::Writer; ArchiveField<TArchive, T>{k, v}; };

template<typename T, typename TArchive>
concept TypeWithReaderSupport = requires(T& v, std::uint8_t k, TArchive& ar) { typename TArchive::Reader; ar.add(ArchiveField<TArchive, T>{k, v}); };

template<typename T, typename TArchive>
concept TypeWithWriterSupport = requires(const T& v, std::uint8_t k, TArchive& ar) { typename TArchive::Writer; ar.add(ArchiveField<TArchive, T>{k, v}); };

template<typename T>
concept BlobType = requires (T& v) {
    typename T::const_iterator;
    typename T::value_type;
} && (std::is_integral_v<typename T::value_type> || std::is_same_v<typename T::value_type, std::byte>)
  && sizeof(typename T::value_type) == 1;


template<typename T>
concept ContainerType = !BlobType<T> && requires (T& v) {
    typename T::const_iterator;
    typename T::value_type;
    { std::begin(v) } -> std::same_as<typename T::iterator>;
    { std::end(v) } -> std::same_as<typename T::iterator>;
};

template <typename T>
concept TupleType = !std::is_reference_v<T> && requires(T t) {
    typename std::tuple_size<T>::type;
    std::get<0>(t);
};

template <typename T>
concept VariantType = !std::is_reference_v<T> && requires(T t) {
    { t.index() } -> std::same_as<std::size_t>;
    std::variant_size<T>::value;
};

template <typename Variant, std::size_t I = 0>
Variant variant_from_index(std::size_t index) {
    assert(index < std::variant_size_v<Variant>);
    if constexpr(I >= std::variant_size_v<Variant>)
        return Variant{};
    else
        return index == 0
               ? Variant{std::in_place_index<I>}
               : variant_from_index<Variant, I + 1>(index - 1);
}


#ifndef XCI_ARCHIVE_NO_MAGIC
template<typename T, typename TArchive>
concept TypeWithMagicSupport =
        !TypeWithSerializeMethod<T, TArchive> &&
        !TypeWithSerializeFunction<T, TArchive> &&
        !TypeWithSchemaMethod<T, TArchive> &&
        !TypeWithSchemaFunction<T, TArchive> &&
        !TypeWithSaveMethod<T, TArchive> &&
        !TypeWithSaveFunction<T, TArchive> &&
        !TypeWithLoadMethod<T, TArchive> &&
        !TypeWithLoadFunction<T, TArchive> &&
        !TypeWithReaderSupport<T, TArchive> &&
        !TypeWithWriterSupport<T, TArchive> &&
        !BlobType<T> &&
        !ContainerType<T> &&
        !TupleType<T> &&
        !VariantType<T> &&
        std::is_class_v<T> &&
        !std::is_reference_v<T> &&
        !std::is_polymorphic_v<T> &&
        std::is_copy_constructible_v<T> &&
        (std::is_aggregate_v<T> || std::is_scalar_v<T>);
#endif

template<typename T>
concept FancyPointerType = requires(T& v) {
    *v;
    bool(v);
    v = nullptr;
} && !std::is_same_v<T, const char*>;

template<typename T>
concept ContainerTypeWithEmplaceBack = ContainerType<T> && requires (T& v) {
    typename T::reference;
    v.emplace_back();
    { v.back() } -> std::same_as<typename T::reference>;
};

template<typename T>
concept ContainerTypeWithEmplace = ContainerType<T> && !ContainerTypeWithEmplaceBack<T> &&
requires (T& v) {
    v.emplace();
};

class ArchiveError : public core::Error {
public:
    explicit ArchiveError(std::string&& msg) : core::Error(std::move(msg)) {}
};


class ArchiveOutOfKeys : public ArchiveError {
public:
    explicit ArchiveOutOfKeys(int key)
        : ArchiveError(fmt::format("Key {} is out of range for the object.", key)) {}
};


class ArchiveKeyNotInOrder : public ArchiveError {
public:
    ArchiveKeyNotInOrder() : ArchiveError("Requested key cannot be allocated (not in order).") {}
};


static constexpr uint8_t key_auto = 255;


template <class TImpl>
class ArchiveBase {
public:
    // convenience: ar(value) -> ar.apply(ArchiveField{key_auto, value})
    template<typename T>
    ArchiveBase& operator() (T&& value) {
        apply(ArchiveField<TImpl, std::remove_cvref_t<T>>{key_auto, std::forward<T>(value)});
        return *this;
    }

    // convenience: ar(key, value) -> ar.apply(ArchiveField{key, value})
    template<typename T>
    ArchiveBase& operator() (uint8_t key, T&& value) {
        apply(ArchiveField<TImpl, std::remove_cvref_t<T>>{key, std::forward<T>(value)});
        return *this;
    }

    // convenience: ar(name, value) -> ar.apply(ArchiveField{key_auto, value, name})
    template<typename T>
    ArchiveBase& operator() (const char* name, T&& value) {
        apply(ArchiveField<TImpl, std::remove_cvref_t<T>>{key_auto, std::forward<T>(value), name});
        return *this;
    }

    // convenience: ar(key, name, value) -> ar.apply(ArchiveField{key, value, name})
    template<typename T>
    ArchiveBase& operator() (uint8_t key, const char* name, T&& value) {
        apply(ArchiveField<TImpl, std::remove_cvref_t<T>>{key, std::forward<T>(value), name});
        return *this;
    }

    // convenience: ar.repeated(value, f_make_value) -> reader.add(ArchiveField{key_auto, value}, f_make_value)
    template <ContainerType T, typename F>
    requires ArchiveIsReader<T, TImpl>
    void repeated(T& value, F&& f_make_value) {
        static_cast<TImpl*>(this)->add(
                ArchiveField<TImpl, T>{static_cast<TImpl*>(this)->draw_next_key(key_auto), value},
                std::forward<F>(f_make_value));
    }

    // when: the type has `serialize` method
    template <TypeWithSerializeMethod<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = static_cast<TImpl*>(this)->draw_next_key(kv.key);
        if (static_cast<TImpl*>(this)->enter_group(kv)) {
            const_cast<T&>(kv.value).serialize(*static_cast<TImpl*>(this));
            static_cast<TImpl*>(this)->leave_group(kv);
        }
    }

    // when: the type has out-of-class serialize() function
    template <TypeWithSerializeFunction<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = static_cast<TImpl*>(this)->draw_next_key(kv.key);
        if (static_cast<TImpl*>(this)->enter_group(kv)) {
            serialize(*static_cast<TImpl*>(this), const_cast<T&>(kv.value));
            static_cast<TImpl*>(this)->leave_group(kv);
        }
    }

    // when: the type has save_schema() method and archive is Schema
    template <TypeWithSchemaMethod<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = static_cast<TImpl*>(this)->draw_next_key(kv.key);
        if (static_cast<TImpl*>(this)->enter_group(kv)) {
            kv.value.save_schema(*static_cast<TImpl*>(this));
            static_cast<TImpl*>(this)->leave_group(kv);
        }
    }

    // when: the type has out-of-class save_schema() function and archive is Schema
    template <TypeWithSchemaFunction<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = static_cast<TImpl*>(this)->draw_next_key(kv.key);
        if (static_cast<TImpl*>(this)->enter_group(kv)) {
            save_schema(*static_cast<TImpl*>(this), kv.value);
            static_cast<TImpl*>(this)->leave_group(kv);
        }
    }

    // when: the type has save() method and this is Writer
    template <TypeWithSaveMethod<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = static_cast<TImpl*>(this)->draw_next_key(kv.key);
        if (static_cast<TImpl*>(this)->enter_group(kv)) {
            kv.value.save(*static_cast<TImpl*>(this));
            static_cast<TImpl*>(this)->leave_group(kv);
        }
    }

    // when: the type has out-of-class save() function and this is Writer
    template <TypeWithSaveFunction<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = static_cast<TImpl*>(this)->draw_next_key(kv.key);
        if (static_cast<TImpl*>(this)->enter_group(kv)) {
            save(*static_cast<TImpl*>(this), kv.value);
            static_cast<TImpl*>(this)->leave_group(kv);
        }
    }

    // when: the type has load() method and this is Reader
    template <TypeWithLoadMethod<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = static_cast<TImpl*>(this)->draw_next_key(kv.key);
        if (static_cast<TImpl*>(this)->enter_group(kv)) {
            kv.value.load(*static_cast<TImpl*>(this));
            static_cast<TImpl*>(this)->leave_group(kv);
        }
    }

    // when: the type has out-of-class load() function and this is Reader
    template <TypeWithLoadFunction<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = static_cast<TImpl*>(this)->draw_next_key(kv.key);
        if (static_cast<TImpl*>(this)->enter_group(kv)) {
            load(*static_cast<TImpl*>(this), kv.value);
            static_cast<TImpl*>(this)->leave_group(kv);
        }
    }

    // when: Archive implementation has add() method for the type
    template <TypeWithReaderSupport<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = static_cast<TImpl*>(this)->draw_next_key(kv.key);
        static_cast<TImpl*>(this)->add(std::forward<ArchiveField<TImpl, T>>(kv));
    }
    template <TypeWithWriterSupport<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = static_cast<TImpl*>(this)->draw_next_key(kv.key);
        static_cast<TImpl*>(this)->add(std::forward<ArchiveField<TImpl, T>>(kv));
    }

    // when: the type is tuple-like (e.g. std::pair)
    template <TupleType T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = static_cast<TImpl*>(this)->draw_next_key(kv.key);
        if (static_cast<TImpl*>(this)->enter_group(kv)) {
            std::apply([this]<typename... Args>(Args&&... args) {
                ((void) (*this)(std::forward<Args>(args)), ...);
            }, kv.value);
            static_cast<TImpl*>(this)->leave_group(kv);
        }
    }

#ifndef XCI_ARCHIVE_NO_MAGIC
    // when: other non-polymorphic structs - use pfr
    template <TypeWithMagicSupport<TImpl> T>
    void apply(ArchiveField<TImpl, T>&& kv) {
        kv.key = static_cast<TImpl*>(this)->draw_next_key(kv.key);
        if (static_cast<TImpl*>(this)->enter_group(kv)) {
            boost::pfr::for_each_field(kv.value, [this](auto& field) {
                (*this)(field);  // operator()
            });
            static_cast<TImpl*>(this)->leave_group(kv);
        }
    }
#endif

};


template <typename BufferType>
class ArchiveGroupStack {
public:
    ArchiveGroupStack() { m_group_stack.emplace_back(); }

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

        if (group.next_key == key_auto)
            throw ArchiveOutOfKeys(group.next_key);

        return group.next_key ++;
    }

    bool is_root_group() const { return m_group_stack.size() == 1; }
    auto& group_buffer() { return m_group_stack.back().buffer; }

    // ===== data =====

    struct Group {
        uint8_t next_key = 0;
        BufferType buffer {};
    };
    std::vector<Group> m_group_stack;
};


} // namespace xci::data

#endif // include guard
