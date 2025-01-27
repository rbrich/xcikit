// WadArchive.cpp created on 2023-11-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "WadArchive.h"
#include <xci/core/log.h>
#include <xci/compat/endian.h>

#include <algorithm>

namespace xci::vfs {

using namespace core::log;


static bool check_wad_magic(const char* magic)
{
    return std::memcmp(magic + 1, "WAD", 3) == 0 && (magic[0] == 'I' || magic[0] == 'P');
}


bool WadArchiveLoader::can_load_stream(std::istream& stream)
{
    std::array<char, 4> magic {};
    stream.seekg(0);
    stream.read(magic.data(), magic.size());
    if (!stream) {
        log::debug("Vfs: WadArchiveLoader: Couldn't read magic: first 4 bytes");
        return false;
    }
    // "IWAD" or "PWAD"
    return check_wad_magic(magic.data());
}


std::shared_ptr<VfsDirectory>
WadArchiveLoader::load_stream(std::string&& path, std::unique_ptr<std::istream>&& stream)
{
    auto archive = std::make_shared<WadArchive>(std::move(path), std::move(stream));
    if (!archive->is_open())
        return {};
    return archive;
}


WadArchive::WadArchive(std::string&& path, std::unique_ptr<std::istream>&& stream)
        : m_path(std::move(path)), m_stream(std::move(stream))
{
    TRACE("Opening archive: {}", m_path);
    // obtain file size
    m_stream->seekg(0, std::ios_base::end);
    const auto size = size_t(m_stream->tellg());
    m_stream->seekg(0);

    // read archive index
    if (!read_index(size)) {
        close_archive();
    }
}


VfsFile WadArchive::read_file(const std::string& path) const
{
    // search for the entry
    auto entry_it = std::find_if(m_entries.cbegin(), m_entries.cend(), [&path](auto& entry){
        return entry.path() == path;
    });
    if (entry_it == m_entries.cend()) {
        log::debug("Vfs: WadArchive: Not found in archive: {}", path);
        return {};
    }

    return read_entry(*entry_it);
}


VfsFile WadArchive::read_entry(const IndexEntry& entry) const
{
    const auto path = entry.path();
    log::debug("Vfs: WadArchive: open file: {}", path);

    // Pass self to Buffer deleter, so the archive object lives
    // at least as long as the buffer.
    auto* content = new std::byte[entry.size];
    BufferPtr buffer_ptr(new Buffer{content, entry.size},
                         [this_ptr = shared_from_this()](Buffer* b){ delete[] b->data(); delete b; });

    m_stream->seekg(entry.filepos);
    m_stream->read((char*) buffer_ptr->data(), std::streamsize(buffer_ptr->size()));
    if (!m_stream) {
        log::error("Vfs: WadArchive: Not found in archive: {}", path);
        return {};
    }

    return VfsFile("", std::move(buffer_ptr));
}


std::string WadArchive::type() const
{
    // read magic from WAD file
    m_stream->seekg(0);
    std::string magic(4, '\0');
    m_stream->read(magic.data(), std::streamsize(magic.size()));
    return magic;
}


unsigned WadArchive::num_entries() const
{
    return m_entries.size();
}


std::string WadArchive::get_entry_name(unsigned index) const
{
    return m_entries[index].path();
}


VfsFile WadArchive::read_entry(unsigned index) const
{
    return read_entry(m_entries[index]);
}


bool WadArchive::read_index(size_t size)
{
    // HEADER: identification
    std::array<char, 4> magic;
    m_stream->read(magic.data(), std::streamsize(magic.size()));
    if (!m_stream || !check_wad_magic(magic.data())) {
        log::error("Vfs: WadArchive: Corrupted archive: {} ({}).",
                   m_path, "identification");
        return false;
    }

    // HEADER: numlumps
    uint32_t num_entries;
    m_stream->read((char*)&num_entries, sizeof(num_entries));
    num_entries = le32toh(num_entries);
    if (!m_stream) {
        log::error("Vfs: WadArchive: Corrupted archive: {} ({}).",
                   m_path, "num entries");
        return false;
    }

    // HEADER: infotableofs
    uint32_t index_offset;
    m_stream->read((char*)&index_offset, sizeof(index_offset));
    index_offset = le32toh(index_offset);
    if (!m_stream || index_offset > size) {
        // the offset must be inside archive
        log::error("Vfs: WadArchive: Corrupted archive: {} ({}).",
                   m_path, "info table offset");
        return false;
    }

    // INDEX (directory)
    m_entries.resize(num_entries);
    m_stream->seekg(index_offset);
    for (auto& entry : m_entries) {
        m_stream->read((char*)&entry, 16);
        entry.filepos = le32toh(entry.filepos);
        entry.size = le32toh(entry.size);
        if (!m_stream || entry.filepos + entry.size > index_offset) {
            log::error("Vfs: WadArchive: Corrupted archive: {} ({}).",
                       m_path, "directory entry");
            return false;
        }
    }
    return true;
}


void WadArchive::close_archive()
{
    if (m_stream) {
        TRACE("Closing archive: {}", m_path);
        m_stream.reset();
    }
}


std::string WadArchive::IndexEntry::path() const
{
    std::string sanitized_name (name, 8);
    sanitized_name.resize(strlen(sanitized_name.c_str()));
    return sanitized_name;
}


}  // namespace xci::vfs
