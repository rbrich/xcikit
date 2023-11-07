// RealDirectory.h created on 2023-11-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_VFS_REAL_DIRECTORY_H
#define XCI_VFS_REAL_DIRECTORY_H

#include "Vfs.h"

namespace xci::vfs {


/// Lookup regular files in real directory, which is mapped to VFS path
class RealDirectoryLoader: public VfsLoader {
public:
    const char* name() const override { return "directory"; }
    bool can_load_fs_dir(const fs::path& path) override { return true; }
    auto load_fs_dir(const fs::path& path) -> std::shared_ptr<VfsDirectory> override;
};


/// Lookup regular files in real directory, which is mapped to VFS path
class RealDirectory: public VfsDirectory {
public:
    explicit RealDirectory(fs::path dir_path) : m_dir_path(std::move(dir_path)) {}

    std::string type() const override { return "DIR"; }

    VfsFile read_file(const std::string& path) const override;

    // Calling any of these methods creates a snapshot of the directory,
    // that is used by subsequent calls. Listing live directory would be fragile.
    unsigned num_entries() const override;
    std::string get_entry_name(unsigned index) const override;
    VfsFile read_entry(unsigned index) const override;

private:
    void snapshot_entries() const;

    fs::path m_dir_path;
    mutable std::vector<fs::path> m_entries;  // snapshot of the directory for listing
};


}  // namespace xci::vfs

#endif  // XCI_VFS_REAL_DIRECTORY_H
