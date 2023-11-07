// ZipArchive.h created on 2023-11-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_VFS_ZIP_ARCHIVE_H
#define XCI_VFS_ZIP_ARCHIVE_H

#include "Vfs.h"

namespace xci::vfs {


/// Lookup files in ZIP archive, which is mapped to VFS path
class ZipArchiveLoader: public VfsLoader {
public:
    const char* name() const override { return "ZIP archive"; }
    bool can_load_stream(std::istream& stream) override;
    auto load_stream(std::string&& path, std::unique_ptr<std::istream>&& stream) -> std::shared_ptr<VfsDirectory> override;
};


/// Lookup files in ZIP archive, which is mapped to VFS path
class ZipArchive: public VfsDirectory {
public:
    explicit ZipArchive(std::string&& path, std::unique_ptr<std::istream>&& stream);
    ~ZipArchive() override;

    std::string type() const override { return "ZIP"; }
    bool is_open() const { return m_zip != nullptr; }

    VfsFile read_file(const std::string& path) const override;
    unsigned num_entries() const override;
    std::string get_entry_name(unsigned index) const override;
    VfsFile read_entry(unsigned index) const override;

private:
    std::string m_path;
    std::unique_ptr<std::istream> m_stream;
    void* m_zip = nullptr;
    size_t m_size = 0;
    int m_last_zip_err = 0;
    int m_last_sys_err = 0;
};


}  // namespace xci::vfs

#endif  // XCI_VFS_ZIP_ARCHIVE_H
