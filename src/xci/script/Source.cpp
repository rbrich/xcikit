// Source.cpp created on 2021-04-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Source.h"
#include <cassert>

namespace xci::script {


const Source& SourceManager::get_source(SourceId id) const
{
    assert(id > 0 && id <= m_sources.size());
    return m_sources[id - 1];
}


SourceId SourceManager::add_source(std::string name, std::string content)
{
    m_sources.emplace_back(std::move(name), std::move(content));
    return (SourceId) m_sources.size();
}


std::string_view SourceLocation::source_name() const
{
    if (source_id == 0)
        return "<no-source-file>";
    assert(source_manager != nullptr);
    return source_manager->get_source(source_id).name();
}


std::string_view SourceLocation::source_line() const
{
    assert(source_manager != nullptr);
    const auto& src = source_manager->get_source(source_id);
    const char* line_begin = src.data() + offset - (column - 1);
    const char* line_end = src.data() + offset;
    while (*line_end != '\n' && *line_end != 0)
        ++line_end;
    return {line_begin, size_t(line_end - line_begin)};
}


} // namespace xci::script
