// DarArchive.cpp created on 2023-11-07 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "DarArchive.h"
#include <xci/core/log.h>
#include <xci/compat/endian.h>

#include <zlib.h>

#include <algorithm>
#include <cassert>

namespace xci::vfs {


static constexpr std::array<char, 4> c_dar_magic = {{'d', 'a', 'r', '1'}};


bool DarArchiveLoader::can_load_stream(std::istream& stream)
{
    std::array<char, c_dar_magic.size()> magic {};
    stream.seekg(0);
    stream.read(magic.data(), magic.size());
    if (!stream) {
        log::debug("Vfs: DarArchiveLoader: couldn't read magic: first {} bytes", c_dar_magic.size());
        return false;
    }
    return magic == c_dar_magic;
}


std::shared_ptr<VfsDirectory>
DarArchiveLoader::load_stream(std::string&& path, std::unique_ptr<std::istream>&& stream)
{
    auto archive = std::make_shared<DarArchive>(std::move(path), std::move(stream));
    if (!archive->is_open())
        return {};
    return archive;
}


DarArchive::DarArchive(std::string&& path, std::unique_ptr<std::istream>&& stream)
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


VfsFile DarArchive::read_file(const std::string& path) const
{
    // search for the entry
    auto entry_it = std::find_if(m_entries.cbegin(), m_entries.cend(), [&path](auto& entry){
        return entry.name == path;
    });
    if (entry_it == m_entries.cend()) {
        log::debug("Vfs: DarArchive: Not found in archive: {}", path);
        return {};
    }

    return read_entry(*entry_it);
}


unsigned DarArchive::num_entries() const
{
    return m_entries.size();
}


std::string DarArchive::get_entry_name(unsigned index) const
{
    return m_entries[index].name;
}


VfsFile DarArchive::read_entry(unsigned index) const
{
    return read_entry(m_entries[index]);
}


VfsFile DarArchive::read_entry(const IndexEntry& entry) const
{
    log::debug("Vfs: DarArchive: open file: {}", entry.name);

    if (entry.encoding() == "--") {
        return read_entry_plain(entry);
    }

    if (entry.encoding() == "zl") {
        return read_entry_zlib(entry);
    }

    log::error("Vfs: DarArchive: Unsupported file encoding \"{}\": {}",
        entry.encoding(), entry.name);
    return {};
}


VfsFile DarArchive::read_entry_plain(const IndexEntry& entry) const
{
    m_stream->seekg(entry.offset);
    BufferPtr buffer_ptr(new Buffer{new std::byte[entry.size], entry.size},
                         [](Buffer* b){ delete[] b->data(); delete b; });

    m_stream->read((char*) buffer_ptr->data(), std::streamsize(buffer_ptr->size()));
    if (!m_stream) {
        log::error("Vfs: DarArchive: Error reading entry: {}", entry.name);
        return {};
    }

    return VfsFile("", std::move(buffer_ptr));
}


VfsFile DarArchive::read_entry_zlib(const IndexEntry& entry) const
{
    if (entry.size < 4) {
        log::error("Vfs: DarArchive: Corrupted zlib entry (missing uncompressed size): {}",
            entry.name);
        return {};
    }

    // Obtain uncompressed size - stored in last 4 bytes of input data
    uint32_t plain_size;
    auto input_size = size_t(entry.size) - sizeof(plain_size);
    m_stream->seekg(std::streamoff(entry.offset + input_size));
    m_stream->read((char*) &plain_size, sizeof(plain_size));
    plain_size = be32toh(plain_size);

    // Allocate buffer for VfsFile
    m_stream->seekg(entry.offset);
    BufferPtr buffer_ptr(new Buffer{new std::byte[plain_size], plain_size},
                         [](Buffer* b){ delete[] b->data(); delete b; });

    // Read and decompress
    z_stream zstream {};
    int zerr = inflateInit2(&zstream, 15);
    if (zerr != Z_OK) {
        log::error("Vfs: DarArchive: inflateInit2: %d", zerr);
        return {};
    }

    char input_buffer[4096];
    zstream.avail_out = plain_size;
    zstream.next_out = (Bytef *) buffer_ptr->data();
    while (input_size > 0) {
        auto read_size = std::min(input_size, sizeof(input_buffer));
        m_stream->read(input_buffer, std::streamsize(read_size));
        if (!m_stream) {
            log::error("Vfs: DarArchive: Error reading entry: {}", entry.name);
            inflateEnd(&zstream);
            return {};
        }

        zstream.avail_in = (uInt) read_size;
        zstream.next_in = (Bytef *) input_buffer;

        zerr = inflate(&zstream, Z_NO_FLUSH);
        if (zerr == Z_STREAM_END) {
            assert(input_size == read_size);
            break;
        }
        if (zerr != Z_OK) {
            log::error("Vfs: DarArchive: inflate: %d", zerr);
            inflateEnd(&zstream);
            return {};
        }

        input_size -= read_size;
    };

    inflateEnd(&zstream);

    return VfsFile("", std::move(buffer_ptr));
}


bool DarArchive::read_index(size_t size)
{
    // HEADER: ID
    std::array<char, c_dar_magic.size()> magic;
    m_stream->read(magic.data(), std::streamsize(magic.size()));
    if (!m_stream || magic != c_dar_magic) {
        log::error("Vfs: DarArchive: Corrupted archive: {} ({})",
                m_path, "ID");
        return false;
    }

    // HEADER: INDEX_OFFSET
    uint32_t index_offset;
    m_stream->read((char*)&index_offset, sizeof(index_offset));
    index_offset = be32toh(index_offset);
    if (!m_stream || index_offset + 4 > size) {
        // the offset must be inside archive, plus 4B for num_entries
        log::error("Vfs: DarArchive: Corrupted archive: {} ({})",
                  m_path, "INDEX_OFFSET");
        return false;
    }

    m_stream->seekg(index_offset);

    // INDEX: INDEX_SIZE
    uint32_t index_size;
    m_stream->read((char*)&index_size, sizeof(index_size));
    index_size = be32toh(index_size);
    if (!m_stream || index_offset + index_size > size) {
        log::error("Vfs: DarArchive: Corrupted archive: {} ({})",
                m_path, "INDEX_SIZE");
        return false;
    }

    // INDEX: NUMBER_OF_ENTRIES
    uint32_t num_entries;
    m_stream->read((char*)&num_entries, sizeof(num_entries));
    num_entries = be32toh(num_entries);
    if (!m_stream) {
        log::error("Vfs: DarArchive: Corrupted archive: {} ({})",
                m_path, "NUMBER_OF_ENTRIES");
        return false;
    }

    // INDEX: INDEX_ENTRY[]
    m_entries.resize(num_entries);
    for (auto& entry : m_entries) {
        struct {
            uint32_t offset;
            uint32_t size;
            uint32_t metadata_size;
            char encoding[2];
            uint16_t name_size;
        } entry_header;
        static_assert(sizeof(entry_header) == 16);
        m_stream->read((char*)&entry_header, sizeof(entry_header));
        if (!m_stream) {
            log::error("Vfs: DarArchive: Corrupted archive: {} ({})",
                      m_path, "INDEX_ENTRY");
            return false;
        }

        // INDEX_ENTRY: CONTENT_OFFSET
        entry.offset = be32toh(entry_header.offset);
        // INDEX_ENTRY: CONTENT_SIZE
        entry.size = be32toh(entry_header.size);
        // INDEX_ENTRY: METADATA_SIZE
        entry.metadata_size = be32toh(entry_header.metadata_size);
        if (entry.offset + entry.size + entry.metadata_size > index_offset) {
            log::error("Vfs: DarArchive: Corrupted archive: {} ({})",
                      m_path, "CONTENT_OFFSET + CONTENT_SIZE + METADATA_SIZE");
            return false;
        }

        // INDEX_ENTRY: ENCODING
        std::memcpy(entry._encoding, entry_header.encoding, sizeof(entry._encoding));
        // INDEX_ENTRY: NAME_SIZE
        auto name_size = be16toh(entry_header.name_size);
        // INDEX_ENTRY: NAME
        entry.name.resize(name_size);
        m_stream->read(entry.name.data(), name_size);
        if (!m_stream) {
            log::error("Vfs: DarArchive: Corrupted archive: {} ({})",
                      m_path, "NAME");
            return false;
        }
    }
    return true;
}


void DarArchive::close_archive()
{
    if (m_stream) {
        TRACE("Closing archive: {}", m_path);
        m_stream.reset();
    }
}


}  // namespace xci::vfs
