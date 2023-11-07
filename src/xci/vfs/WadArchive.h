// WadArchive.h created on 2023-11-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_VFS_WAD_ARCHIVE_H
#define XCI_VFS_WAD_ARCHIVE_H

#include "Vfs.h"

namespace xci::vfs {


/// Lookup files in WAD file (DOOM 1 format), which is mapped to VFS path
class WadArchiveLoader: public VfsLoader {
public:
    const char* name() const override { return "WAD file"; }
    bool can_load_stream(std::istream& stream) override;
    auto load_stream(std::string&& path, std::unique_ptr<std::istream>&& stream) -> std::shared_ptr<VfsDirectory> override;
};


/// Lookup files in WAD file, which is mapped to VFS path
/// WAD is DOOM 1 uncompressed data file format.
/// Same as DarArchive, this has no external dependency and very simple implementation.
/// Unlike DarArchive, WAD depends on lump order (lump = archived file), lump names can repeat
/// and they are limited to 8 chars.
/// When looking up files by name, only the first lump of that name is returned.
/// Use entry listing to process lumps in order.
/// Reference: https://doomwiki.org/wiki/WAD
class WadArchive: public VfsDirectory {
    friend class WadArchiveLoader;
public:
    explicit WadArchive(std::string&& path, std::unique_ptr<std::istream>&& stream);
    ~WadArchive() override { close_archive(); }

    std::string type() const override;  // "IWAD" or "PWAD"
    bool is_open() const { return bool(m_stream); }

    VfsFile read_file(const std::string& path) const override;
    unsigned num_entries() const override;
    std::string get_entry_name(unsigned index) const override;
    VfsFile read_entry(unsigned index) const override;

private:
    struct IndexEntry {
        uint32_t filepos;
        uint32_t size;
        char name[8];  // not zero-terminated, use path() instead

        std::string path() const;
    };
    static_assert(sizeof(IndexEntry) == 16);

    bool read_index(size_t size);
    VfsFile read_entry(const IndexEntry& entry) const;
    void close_archive();

    std::string m_path;
    std::unique_ptr<std::istream> m_stream;

    // index:
    std::vector<IndexEntry> m_entries;
};


}  // namespace xci::vfs

#endif  // XCI_VFS_WAD_ARCHIVE_H
