// Vfs.cpp created on 2018-09-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Vfs.h"
#include "file.h"
#include <xci/core/log.h>
#include <xci/core/string.h>
#include <xci/core/sys.h>
#include <xci/compat/endian.h>
#include <xci/compat/macros.h>

#include <zip.h>

#include <map>
#include <sstream>
#include <algorithm>
#include <cstddef>  // byte
#include <fcntl.h>

// Deprecated, but not easily replaceable - need to convert existing buffer to a stream, without copying
// Possible future C++ replacement is "spanstream",
// see http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2139r2.html#3.12
// GCC: pragma diagnostic doesn't disable the warning, let's use this ugly hack...
#define _BACKWARD_BACKWARD_WARNING_H 1  // NOLINT
#include <strstream>
#undef _BACKWARD_BACKWARD_WARNING_H

namespace xci::core {

using namespace core::log;


namespace vfs {

// -------------------------------------------------------------------------------------------------
// Real directory

auto RealDirectoryLoader::load_fs_dir(const fs::path& path) -> std::shared_ptr<VfsDirectory>
{
    return std::make_shared<vfs::RealDirectory>(path);
}


VfsFile RealDirectory::read_file(const std::string& path) const
{
    auto full_path = m_dir_path / path;
    log::debug("VfsDirLoader: open file: {}", full_path);

    // open the file
    auto buffer_ptr = read_binary_file(full_path);
    if (!buffer_ptr) {
        log::error("VfsDirLoader: Failed to read file: {}: {m}", full_path);
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


// -------------------------------------------------------------------------------------------------
// DAR archive

static constexpr std::array<char, 4> c_dar_magic = {{'d', 'a', 'r', '\n'}};


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
        log::error("Vfs: DarArchive: Not found in archive: {}", path);
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
    // return a view into mmapped archive
    log::debug("Vfs: DarArchive: open file: {}", entry.name);

    // Pass self to Buffer deleter, so the archive object lives
    // at least as long as the buffer.
    auto* content = new std::byte[entry.size];
    BufferPtr buffer_ptr(new Buffer{content, entry.size},
                         [this_ptr = shared_from_this()](Buffer* b){ delete[] b->data(); delete b; });

    m_stream->seekg(entry.offset);
    m_stream->read((char*) buffer_ptr->data(), std::streamsize(buffer_ptr->size()));
    if (!m_stream) {
        log::error("Vfs: DarArchive: Not found in archive: {}", entry.name);
        return {};
    }

    return VfsFile("", std::move(buffer_ptr));
}


bool DarArchive::read_index(size_t size)
{
    // HEADER: ID
    std::array<char, c_dar_magic.size()> magic;
    m_stream->read(magic.data(), std::streamsize(magic.size()));
    if (!m_stream || magic != c_dar_magic) {
        log::error("Vfs: DarArchive: Corrupted archive: {} ({}).",
                m_path, "ID");
        return false;
    }

    // HEADER: INDEX_OFFSET
    uint32_t index_offset;
    m_stream->read((char*)&index_offset, sizeof(index_offset));
    index_offset = be32toh(index_offset);
    if (!m_stream || index_offset + 4 > size) {
        // the offset must be inside archive, plus 4B for num_entries
        log::error("Vfs: DarArchive: Corrupted archive: {} ({}).",
                  m_path, "INDEX_OFFSET");
        return false;
    }

    // INDEX: NUMBER_OF_ENTRIES
    m_stream->seekg(index_offset);
    uint32_t num_entries;
    m_stream->read((char*)&num_entries, sizeof(num_entries));
    num_entries = be32toh(num_entries);
    if (!m_stream) {
        log::error("Vfs: DarArchive: Corrupted archive: {} ({}).",
                m_path, "INDEX_ENTRY");
        return false;
    }

    // INDEX: INDEX_ENTRY[]
    m_entries.resize(num_entries);
    for (auto& entry : m_entries) {
        struct {
            uint32_t offset;
            uint32_t size;
            uint16_t name_size;
        } entry_header;
        m_stream->read((char*)&entry_header, 10);  // sizeof would be 12 due to padding
        if (!m_stream) {
            log::error("Vfs: DarArchive: Corrupted archive: {} ({}).",
                      m_path, "INDEX_ENTRY");
            return false;
        }

        // INDEX_ENTRY: CONTENT_OFFSET
        entry.offset = be32toh(entry_header.offset);
        // INDEX_ENTRY: CONTENT_SIZE
        entry.size = be32toh(entry_header.size);
        if (entry.offset + entry.size > index_offset) {
            // there must be space for the name in archive
            log::error("Vfs: DarArchive: Corrupted archive: {} ({}).",
                      m_path, "CONTENT_OFFSET + CONTENT_SIZE");
            return false;
        }

        // INDEX_ENTRY: NAME_SIZE
        auto name_size = be16toh(entry_header.name_size);
        // INDEX_ENTRY: NAME
        entry.name.resize(name_size);
        m_stream->read(entry.name.data(), name_size);
        if (!m_stream) {
            log::error("Vfs: DarArchive: Corrupted archive: {} ({}).",
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


// -------------------------------------------------------------------------------------------------
// WAD file

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
        log::debug("Vfs: WadArchiveLoader: couldn't read magic: first 4 bytes");
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
        log::error("Vfs: WadArchive: Not found in archive: {}", path);
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


// -------------------------------------------------------------------------------------------------
// ZIP archive

bool ZipArchiveLoader::can_load_stream(std::istream& stream)
{
    char magic[2];
    stream.seekg(0);
    stream.read(magic, sizeof(magic));
    if (!stream) {
        log::debug("Vfs: ZipArchiveLoader: couldn't read magic: first 2 bytes");
        return {};
    }
    return (magic[0] == 'P' && magic[1] == 'K');
}

std::shared_ptr<VfsDirectory>
ZipArchiveLoader::load_stream(std::string&& path, std::unique_ptr<std::istream>&& stream)
{
    auto archive = std::make_shared<ZipArchive>(std::move(path), std::move(stream));
    if (!archive->is_open())
        return {};
    return archive;
}


ZipArchive::ZipArchive(std::string&& path, std::unique_ptr<std::istream>&& stream)
    : m_path(std::move(path)), m_stream(std::move(stream))
{
    TRACE("ZipArchive: Opening archive: {}", m_path);

    // obtain file size
    m_stream->seekg(0, std::ios_base::end);
    m_size = size_t(m_stream->tellg());
    m_stream->seekg(0);
    // reset errno - it will be read in the source callback
    errno = 0;
    zip_error_t err {};
    zip_source_t* zip_source = zip_source_function_create(
            [](void *userdata, void *data, zip_uint64_t len, zip_source_cmd_t cmd) -> zip_int64_t {
                auto* self = static_cast<ZipArchive*>(userdata);
                auto& sp = self->m_stream;
                struct Finally {
                    ~Finally() { self->m_last_sys_err = errno; }
                    ZipArchive* self;
                };
                Finally finally;
                finally.self = self;
                switch (cmd) {
                    case ZIP_SOURCE_OPEN:
                        if (!sp) {
                            self->m_last_zip_err = ZIP_ER_OPEN;
                            return -1;
                        }
                        return 0;
                    case ZIP_SOURCE_READ:
                        if (!sp) {
                            self->m_last_zip_err = ZIP_ER_INTERNAL;
                            return -1;
                        }
                        sp->read((char*)data, std::streamsize(len));
                        if (sp->eof())
                            return 0;
                        if (sp->fail()) {
                            self->m_last_zip_err = ZIP_ER_READ;
                            return -1;
                        }
                        return sp->gcount();
                    case ZIP_SOURCE_CLOSE:
                        if (sp)
                            sp.reset();
                        return 0;
                    case ZIP_SOURCE_STAT: {
                        auto* st = static_cast<zip_stat_t*>(data);
                        zip_stat_init(st);
                        st->name = self->m_path.c_str();
                        st->size = self->m_size;
                        st->valid = ZIP_STAT_SIZE | ZIP_STAT_NAME;
                        return sizeof(zip_stat_t);
                    }
                    case ZIP_SOURCE_ERROR: {
                        auto* err = static_cast<zip_error_t*>(data);
                        err->zip_err = self->m_last_zip_err;
                        err->sys_err = self->m_last_sys_err;
                        self->m_last_zip_err = ZIP_ER_OK;
                        return 0;
                    }
                    case ZIP_SOURCE_SEEK: {
                        if (!sp) {
                            self->m_last_zip_err = ZIP_ER_INTERNAL;
                            return -1;
                        }
                        if (len < sizeof(zip_source_args_seek_t)) {
                            self->m_last_zip_err = ZIP_ER_INVAL;
                            return -1;
                        }
                        auto* args = static_cast<zip_source_args_seek_t*>(data);
                        std::ios_base::seekdir dir;
                        switch (args->whence) {
                            case SEEK_SET:  dir = std::ios_base::beg; break;
                            case SEEK_CUR:  dir = std::ios_base::cur; break;
                            case SEEK_END:  dir = std::ios_base::end; break;
                            default:
                                self->m_last_zip_err = ZIP_ER_INVAL;
                                return -1;
                        }
                        sp->seekg(args->offset, dir);
                        if (sp->fail()) {
                            self->m_last_zip_err = ZIP_ER_SEEK;
                            return -1;
                        }
                        return 0;
                    }
                    case ZIP_SOURCE_TELL:
                        if (!sp) {
                            self->m_last_zip_err = ZIP_ER_INTERNAL;
                            return -1;
                        }
                        return sp->tellg();
                    case ZIP_SOURCE_FREE:
                        return 0;
                    case ZIP_SOURCE_SUPPORTS:
                        return ZIP_SOURCE_SUPPORTS_SEEKABLE;
                    default:
                        self->m_last_zip_err = ZIP_ER_OPNOTSUPP;
                        return -1;
                }
                return 0;
            }, this, &err);
    if (zip_source == nullptr) {
        log::error("ZipArchive: zip_source_function_create: {}: {}", m_path, err.zip_err);
        return;
    }

    // open archive file
    zip_t* f = zip_open_from_source(zip_source, ZIP_RDONLY, &err);
    if (f == nullptr) {
        zip_source_free(zip_source);
        log::error("ZipArchive: Failed to open archive: {}: {}", m_path, zip_error_strerror(&err));
        return;
    }

    m_zip = f;
}


ZipArchive::~ZipArchive()
{
    if (m_zip != nullptr) {
        TRACE("ZipArchive: Closing archive: {}", m_path);
        zip_close((zip_t*) m_zip);
    }
}


VfsFile ZipArchive::read_file(const std::string& path) const
{
    if (!is_open()) {
        log::error("ZipArchive: Cannot read - archive is not open");
        return {};
    }

    struct zip_stat st = {};
    zip_stat_init(&st);
    if (zip_stat((zip_t*) m_zip, path.c_str(), ZIP_FL_ENC_RAW, &st) == -1) {
        auto* err = zip_get_error((zip_t*) m_zip);
        if (err->zip_err == ZIP_ER_NOENT) {
            log::error("ZipArchive: Not found in archive: {}", path);
        } else {
            log::error("ZipArchive: Cannot read: {}: {}", path, err->str);
        }
        return {};
    }
    if ((st.valid & ZIP_STAT_SIZE) != ZIP_STAT_SIZE) {
        log::error("ZipArchive: Cannot read: {} (missing size?!)", path);
        return {};
    }

    auto* data = new std::byte[st.size];
    BufferPtr buffer_ptr(new Buffer{data, size_t(st.size)},
            [](Buffer* b){ delete[] b->data(); delete b; });

    zip_file* f = zip_fopen_index((zip_t*) m_zip, st.index, 0);
    if (f == nullptr) {
        auto* err = zip_get_error((zip_t*) m_zip);
        log::error("ZipArchive: zip_fopen_index({}): {}", path, err->str);
        return {};
    }
    zip_int64_t nbytes = zip_fread(f, data, st.size);
    zip_fclose(f);
    if (nbytes < 0 || (zip_uint64_t)nbytes != st.size) {
        log::error("ZipArchive: Cannot read: {}: Read {} bytes of {}",
                path, nbytes, st.size);
        return {};
    }

    return VfsFile("", std::move(buffer_ptr));
}


unsigned ZipArchive::num_entries() const
{
    return zip_get_num_entries((zip_t*) m_zip, 0);
}


std::string ZipArchive::get_entry_name(unsigned index) const
{
    return zip_get_name((zip_t*) m_zip, index, 0);
}


VfsFile ZipArchive::read_entry(unsigned index) const
{
    return read_file(get_entry_name(index));
}



// -------------------------------------------------------------------------------------------------

}  // namespace vfs


Vfs::Vfs(Loaders loaders)
{
    switch (loaders) {
        case Loaders::All:
            m_loaders.emplace_back(std::make_unique<vfs::ZipArchiveLoader>());
            [[fallthrough]];
        case Loaders::NoZip:
            m_loaders.emplace_back(std::make_unique<vfs::DarArchiveLoader>());
            m_loaders.emplace_back(std::make_unique<vfs::WadArchiveLoader>());
            [[fallthrough]];
        case Loaders::NoArchives:
            m_loaders.emplace_back(std::make_unique<vfs::RealDirectoryLoader>());
            [[fallthrough]];
        case Loaders::None:
            break;
    }
}


bool Vfs::mount(const fs::path& fs_path, std::string target_path)
{
    fs::path real_path;
    if (fs_path.is_relative()) {
        // handle relative path - it's relative to program executable,
        // or its parent, or its parent's parent. The nearest matched parent wins.
        auto base_dir = self_executable_path().parent_path();
        bool found = false;
        for (int parent = 0; parent < 5; ++parent) {
            std::error_code err;
            real_path = fs::canonical(base_dir / fs_path, err);
            if (!err) {
                // found an existing path
                found = true;
                break;
            }
            base_dir = base_dir.parent_path();
        }
        if (!found) {
            real_path = fs_path;  // keep it as relative path in respect to CWD
        }
    } else {
        real_path = fs_path;
    }

    std::shared_ptr<VfsDirectory> vfs_directory;
    const char* loader_name = nullptr;

    if (fs::is_directory(real_path)) {
        // Real directory - try each loader
        for (auto& loader : m_loaders) {
            if (!loader->can_load_fs_dir(real_path))
                continue;
            vfs_directory = loader->load_fs_dir(real_path);
            loader_name = loader->name();
            break;
        }
    } else {
        // Not directory - open as regular file
        auto f = std::make_unique<std::ifstream>(real_path, std::ios::binary);
        if (!*f) {
            log::error("Vfs: failed to open file {}: {m}", real_path);
            return false;
        }
        // Try each loader
        for (auto& loader : m_loaders) {
            if (!loader->can_load_stream(*f))
                continue;
            vfs_directory = loader->load_stream(real_path.string(), std::move(f));
            loader_name = loader->name();
            break;
        }
    }

    if (!vfs_directory) {
        log::warning("Vfs: no loader found for {}", real_path);
        return false;
    }

    // Success, record the mounted dir
    lstrip(target_path, '/');
    rstrip(target_path, '/');
    log::info("Vfs: mounted {} '{}' to /{}", loader_name, real_path, target_path);
    m_mounted_dir.push_back({std::move(target_path), std::move(vfs_directory)});
    return true;
}


bool Vfs::mount_memory(const std::byte* data, size_t size, std::string target_path)
{
    XCI_IGNORE_DEPRECATED(
    auto stream = std::make_unique<std::istrstream>((const char*)(data), size);
    )
    auto path = fmt::format("memory:{:x},{}", intptr_t(data), size);
    std::shared_ptr<VfsDirectory> vfs_directory;

    // Try each loader
    const char* loader_name = nullptr;
    for (auto& loader : m_loaders) {
        if (!loader->can_load_stream(*stream))
            continue;
        vfs_directory = loader->load_stream(std::string(path), std::move(stream));
        loader_name = loader->name();
        break;
    }

    if (!vfs_directory) {
        log::warning("Vfs: no loader found for {}", path);
        return false;
    }

    // Success, record the mounted dir
    lstrip(target_path, '/');
    rstrip(target_path, '/');
    log::info("Vfs: mounted {} '{}' to /{}", loader_name, path, target_path);
    m_mounted_dir.push_back({std::move(target_path), std::move(vfs_directory)});
    return true;
}


VfsFile Vfs::read_file(std::string path) const
{
    lstrip(path, '/');
    log::debug("Vfs: try open: {}", path);
    for (const auto& path_loader : m_mounted_dir) {
        // Is the loader applicable for requested path?
        if (!path_loader.path.empty()) {
            if (!remove_prefix(path, path_loader.path))
                continue;
            if (path.front() != '/')
                continue;
            lstrip(path, '/');
        }
        // Open the path with loader
        auto f = path_loader.vfs_dir->read_file(path);
        if (f.is_open())
            return f;
    }
    log::debug("Vfs: failed to open file");
    return {};
}


}  // namespace xci::core
