// RealDirectory.cpp created on 2023-11-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "RealDirectory.h"
#include <xci/core/file.h>
#include <xci/core/log.h>

namespace xci::vfs {

using namespace core::log;


auto RealDirectoryLoader::load_fs_dir(const fs::path& path) -> std::shared_ptr<VfsDirectory>
{
    return std::make_shared<RealDirectory>(path);
}


VfsFile RealDirectory::read_file(const std::string& path) const
{
    auto full_path = m_dir_path / path;

    // open the file
    auto buffer_ptr = read_binary_file(full_path);
    if (!buffer_ptr) {
        log::debug("Vfs: RealDirectory: Failed to read file: {}: {m}", full_path);
        return {};
    }

    return VfsFile(std::move(full_path), std::move(buffer_ptr));
}


unsigned RealDirectory::num_entries() const
{
    snapshot_entries();
    return m_entries.size();
}


std::string RealDirectory::get_entry_name(unsigned index) const
{
    snapshot_entries();
    return m_entries[index].string();
}


VfsFile RealDirectory::read_entry(unsigned index) const
{
    snapshot_entries();
    return read_file(get_entry_name(index));
}


void RealDirectory::snapshot_entries() const
{
    if (!m_entries.empty())
        return;
    for (const auto& entry : fs::recursive_directory_iterator{m_dir_path}) {
        m_entries.push_back(entry.path());
    }
}


}  // namespace xci::vfs
