// NameId.h created on 2023-09-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_NAME_ID_H
#define XCI_SCRIPT_NAME_ID_H

#include <xci/core/container/StringPool.h>
#include <string>
#include <string_view>

namespace xci::script {


// Strong type encapsulating the StringPool::Id
// Convenient for use in parameters etc.
class NameId {
public:
    using Id = core::StringPool::Id;
    static constexpr Id empty_string = core::StringPool::empty_string;

    NameId() = default;
    NameId(Id id) : m_id(id) {}

    // Thread-local string pool for interned strings (names, symbols)
    // Note that the compiler is single-thread, so each compiler has its own instance of StringPool.
    static core::StringPool& string_pool();

    // This is costly operation, keep it explicit - don't add it to a constructor
    static NameId intern(std::string_view name) { return string_pool().add(name); }

    friend bool operator==(NameId a, NameId b) { return a.m_id == b.m_id; }
    friend bool operator<(NameId a, NameId b) { return a.m_id < b.m_id; }

    [[nodiscard]] Id id() const { return m_id; }
    [[nodiscard]] bool empty() const { return m_id == empty_string; }
    explicit operator bool() const { return m_id != empty_string; }

    [[nodiscard]] std::string str() const { return std::string{view()}; }
    [[nodiscard]] std::string_view view() const { return string_pool().view(m_id); }

    template<class Archive>
    void save(Archive& ar) const {
        ar("name", str());
    }

    template<class Archive>
    void load(Archive& ar) {
        std::string name;
        ar("name", name);
        m_id = intern(name).id();
    }

private:
    Id m_id = empty_string;
};


inline NameId intern(std::string_view name) { return NameId::intern(name); }
inline auto format_as(NameId s) { return s.view(); }


} // namespace xci::script

#endif  // include guard
