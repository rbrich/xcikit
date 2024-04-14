// DarArchive.h created on 2023-11-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_VFS_DAR_ARCHIVE_H
#define XCI_VFS_DAR_ARCHIVE_H

#include "Vfs.h"
#include <string_view>

namespace xci::vfs {


/// Lookup files in DAR archive, which is mapped to VFS path
class DarArchiveLoader: public VfsLoader {
public:
    const char* name() const override { return "DAR archive"; }
    bool can_load_stream(std::istream& stream) override;
    auto load_stream(std::string&& path, std::unique_ptr<std::istream>&& stream) -> std::shared_ptr<VfsDirectory> override;
};


/// Lookup files in DAR archive, which is mapped to VFS path
/// DAR is custom uncompressed archive format, see `tools/pack_assets.py`
/// Unlike ZipArchive, this has no external dependency and very simple implementation.
class DarArchive: public VfsDirectory {
public:
    explicit DarArchive(std::string&& path, std::unique_ptr<std::istream>&& stream);
    ~DarArchive() override { close_archive(); }

    std::string type() const override { return "DAR"; }

    bool is_open() const { return bool(m_stream); }

    VfsFile read_file(const std::string& path) const override;
    unsigned num_entries() const override;
    std::string get_entry_name(unsigned index) const override;
    VfsFile read_entry(unsigned index) const override;

private:
    struct IndexEntry {
        uint32_t offset;
        uint32_t size;
        uint32_t metadata_size;
        char _encoding[2];
        std::string name;

        std::string_view encoding() const { return {_encoding, 2}; }
    };

    bool read_index(size_t size);
    VfsFile read_entry(const IndexEntry& entry) const;
    void close_archive();

    std::string m_path;
    std::unique_ptr<std::istream> m_stream;

    // index:
    std::vector<IndexEntry> m_entries;
};


}  // namespace xci::vfs

#endif  // XCI_VFS_DAR_ARCHIVE_H
