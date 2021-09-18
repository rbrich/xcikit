// Source.h created on 2019-12-25 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_SOURCE_H
#define XCI_SCRIPT_SOURCE_H

#include <vector>
#include <string>
#include <cstddef>

namespace xci::script {

using std::move;

using SourceId = uint32_t;
class SourceManager;


// Source location
// Offset into source code, used to print error context
struct SourceLocation {
    const SourceManager* source_manager = nullptr;
    SourceId source_id = 0;     // 0 = unknown
    unsigned line = 0;          // 1-based, 0 = invalid
    unsigned column = 0;        // 1-based, 0 = invalid
    unsigned offset = 0;        // 0-based offset in file

    template <typename Input, typename Position>
    void load(const Input &in, const Position& pos) {
        // in.source() is SourceRef (see below)
        source_manager = &in.source().source_manager;
        source_id = in.source().source_id;
        line = (unsigned) pos.line;
        column = (unsigned) pos.column;
        offset = (unsigned) pos.byte;
    }

    std::string_view source_name() const;
    std::string_view source_line() const;

    explicit operator bool() const { return source_id != 0; }
};


class Source {
public:
    Source(std::string&& name, std::string&& content)
        : m_name(move(name)), m_content(move(content)) {}

    const std::string& name() const { return m_name; }
    const char* data() const { return m_content.data(); }
    size_t size() const { return m_content.size(); }

private:
    std::string m_name;
    std::string m_content;
};


class SourceManager {
public:
    SourceId add_source(std::string name, std::string content);
    const Source& get_source(SourceId id) const;

private:
    std::vector<Source> m_sources;  // SourceId - 1 => index
};


struct SourceRef {
    const SourceManager& source_manager;
    SourceId source_id;

    const Source& source() const { return source_manager.get_source(source_id); }

    // For use as Source in tao::pegtl::memory_input,
    // it needs to be convertible to string for tao::pegtl::position
    operator std::string() const { return source().name(); }
};


} // namespace xci::script

#endif // include guard
