// Vfs.cpp created on 2018-09-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Vfs.h"
#include "file.h"
#include <xci/core/log.h>
#include <xci/core/string.h>
#include <xci/core/sys.h>
#include <xci/core/file.h>
#include <xci/compat/endian.h>
#include <xci/compat/macros.h>

#ifdef XCI_WITH_ZIP
#include <zip.h>
#endif

#include <algorithm>
#include <cstddef>  // byte
#include <fcntl.h>

namespace xci::core {

using namespace core::log;


std::shared_ptr<VfsDirectory>
vfs::RealDirectoryLoader::try_load(const fs::path& path, bool is_dir, Magic)
{
    if (!is_dir)
        return {};
    return std::make_shared<vfs::RealDirectory>(path);
}


VfsFile vfs::RealDirectory::read_file(const std::string& path)
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


// ----------------------------------------------------------------------------


static constexpr std::array<char, 4> c_dar_magic = {{'d', 'a', 'r', '\n'}};


std::shared_ptr<VfsDirectory>
vfs::DarArchiveLoader::try_load(const fs::path& path, bool is_dir, Magic magic)
{
    if (is_dir || magic != c_dar_magic)
        return {};
    return std::make_shared<vfs::DarArchive>(path);
}


vfs::DarArchive::DarArchive(fs::path path)
    : m_archive_path(std::move(path))
{
    TRACE("Opening archive: {}", m_archive_path);
    m_archive.open(m_archive_path, std::ios::in | std::ios::ate | std::ios::binary);
    if (!m_archive) {
        log::error("VfsDarArchiveLoader: Failed to open file: {}: {m}", m_archive_path);
        return;
    }

    // obtain file size
    size_t size = size_t(m_archive.tellg());
    m_archive.seekg(0);

    // read archive index
    if (!read_index(size)) {
        close_archive();
    }
}


VfsFile vfs::DarArchive::read_file(const std::string& path)
{
    // search for the entry
    auto entry_it = std::find_if(m_entries.cbegin(), m_entries.cend(), [&path](auto& entry){
        return entry.name == path;
    });
    if (entry_it == m_entries.cend()) {
        log::error("VfsDarArchiveLoader: Not found in archive: {}", path);
        return {};
    }

    // return a view into mmapped archive
    log::debug("VfsDarArchiveLoader: open file: {}", path);

    // Pass self to Buffer deleter, so the archive object lives
    // at least as long as the buffer.
    auto* content = new std::byte[entry_it->size];
    BufferPtr buffer_ptr(new Buffer{content, entry_it->size},
            [this_ptr = shared_from_this()](Buffer* b){ delete[] b->data(); delete b; });

    m_archive.seekg(entry_it->offset);
    m_archive.read((char*) buffer_ptr->data(), buffer_ptr->size());
    if (!m_archive) {
        log::error("VfsDarArchiveLoader: Not found in archive: {}", path);
        return {};
    }

    return VfsFile("", std::move(buffer_ptr));
}


bool vfs::DarArchive::read_index(size_t size)
{
    // HEADER: ID
    std::array<char, c_dar_magic.size()> magic;
    m_archive.read(magic.data(), magic.size());
    if (!m_archive || magic != c_dar_magic) {
        log::error("VfsDarArchiveLoader: Corrupted archive: {} ({}).",
                m_archive_path, "ID");
        return false;
    }

    // HEADER: INDEX_OFFSET
    uint32_t index_offset;
    m_archive.read((char*)&index_offset, sizeof(index_offset));
    if (!m_archive || be32toh(index_offset) + 4 > size) {
        // the offset must be inside archive, plus 4B for num_entries
        log::error("VfsDarArchiveLoader: Corrupted archive: {} ({}).",
                  m_archive_path, "INDEX_OFFSET");
        return false;
    }
    index_offset = be32toh(index_offset);

    // INDEX: NUMBER_OF_ENTRIES
    m_archive.seekg(index_offset);
    uint32_t num_entries;
    m_archive.read((char*)&num_entries, sizeof(num_entries));
    if (!m_archive) {
        log::error("VfsDarArchiveLoader: Corrupted archive: {} ({}).",
                m_archive_path, "INDEX_ENTRY");
        return false;
    }
    num_entries = be32toh(num_entries);

    // INDEX: INDEX_ENTRY[]
    m_entries.resize(num_entries);
    for (auto& entry : m_entries) {
        struct {
            uint32_t offset;
            uint32_t size;
            uint16_t name_size;
        } entry_header;
        m_archive.read((char*)&entry_header, 10);  // sizeof would be 12 due to padding
        if (!m_archive) {
            log::error("VfsDarArchiveLoader: Corrupted archive: {} ({}).",
                      m_archive_path, "INDEX_ENTRY");
            return false;
        }

        // INDEX_ENTRY: CONTENT_OFFSET
        entry.offset = be32toh(entry_header.offset);
        // INDEX_ENTRY: CONTENT_SIZE
        entry.size = be32toh(entry_header.size);
        if (entry.offset + entry.size > index_offset) {
            // there must be space for the name in archive
            log::error("VfsDarArchiveLoader: Corrupted archive: {} ({}).",
                      m_archive_path, "CONTENT_OFFSET + CONTENT_SIZE");
            return false;
        }

        // INDEX_ENTRY: NAME_SIZE
        auto name_size = be16toh(entry_header.name_size);
        // INDEX_ENTRY: NAME
        entry.name.resize(name_size);
        m_archive.read(&entry.name[0], name_size);
        if (!m_archive) {
            log::error("VfsDarArchiveLoader: Corrupted archive: {} ({}).",
                      m_archive_path, "NAME");
            return false;
        }
    }
    return true;
}


void vfs::DarArchive::close_archive()
{
    if (m_archive.is_open()) {
        TRACE("Closing archive: {}", m_archive_path);
        m_archive.close();
    }
}


// ----------------------------------------------------------------------------


std::shared_ptr<VfsDirectory>
vfs::ZipArchiveLoader::try_load(const fs::path& path, bool is_dir, Magic magic)
{
    if (is_dir || magic[0] != 'P' || magic[1] != 'K')
        return {};

    auto zip = std::make_shared<vfs::ZipArchive>(path);
    if (!zip->is_open())
        return {};
    return zip;
}


vfs::ZipArchive::ZipArchive(fs::path path)
    : m_zip_path(std::move(path))
{
    TRACE("ZipArchive: Opening archive: {}", m_zip_path);
#ifdef XCI_WITH_ZIP
    // open archive file
    int err = 0;
    zip_t* f = zip_open(m_zip_path.c_str(), ZIP_RDONLY, &err);
    if (f == nullptr) {
        log::error("ZipArchive: Failed to open archive: {}: {}", m_zip_path, err);
        return;
    }

    m_zip = f;
#else
    log::error("ZipArchive: Not supported (not compiled with XCI_WITH_ZIP)");
#endif
}


vfs::ZipArchive::~ZipArchive()
{
#ifdef XCI_WITH_ZIP
    if (m_zip != nullptr) {
        TRACE("ZipArchive: Closing archive: {}", m_zip_path);
        zip_close((zip_t*) m_zip);
    }
#endif
}


VfsFile vfs::ZipArchive::read_file(const std::string& path)
{
#ifdef XCI_WITH_ZIP
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
    BufferPtr buffer_ptr(new Buffer{data, st.size},
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
#else
    UNUSED path;
    log::error("ZipArchive: Not supported (not compiled with XCI_WITH_ZIP)");
    return {};
#endif
}


// ----------------------------------------------------------------------------


Vfs::Vfs(Loaders loaders)
{
    switch (loaders) {
        case Loaders::All:
            m_loaders.emplace_back(std::make_unique<vfs::ZipArchiveLoader>());
            FALLTHROUGH;
        case Loaders::NoZip:
            m_loaders.emplace_back(std::make_unique<vfs::DarArchiveLoader>());
            FALLTHROUGH;
        case Loaders::NoArchives:
            m_loaders.emplace_back(std::make_unique<vfs::RealDirectoryLoader>());
            FALLTHROUGH;
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
        for (int parent = 0; parent < 5; ++parent) {
            std::error_code err;
            real_path = fs::canonical(base_dir / fs_path, err);
            if (!err)
                break;  // found an existing path
            base_dir = base_dir.parent_path();
        }
    } else {
        real_path = fs_path;
    }

    // Check path type
    std::error_code err;
    auto st = fs::status(real_path, err);
    if (err) {
        log::warning("Vfs: couldn't mount {}: {}", real_path, err.message());
        return false;
    }

    // Read magic bytes in case of regular file
    VfsLoader::Magic magic {};
    if (fs::is_regular_file(st)) {
        std::ifstream f(real_path, std::ios::binary);
        if (!f) {
            log::warning("Vfs: couldn't mount {}: {m}", real_path);
            return false;
        }
        f.read(magic.data(), magic.size());
        if (!f) {
            log::warning("Vfs: couldn't mount {}: couldn't read magic bytes ({}B)",
                real_path, magic.size());
            return false;
        }
    }

    // Try each loader
    bool is_dir = fs::is_directory(st);
    std::shared_ptr<VfsDirectory> vfs_directory;
    for (auto& loader : m_loaders) {
        vfs_directory = loader->try_load(real_path, is_dir, magic);
        if (!vfs_directory)
            continue;
        log::info("Vfs: mount {} ({})", real_path, loader->name());
        break;
    }
    if (!vfs_directory) {
        log::warning("Vfs: couldn't mount {}", real_path);
        return false;
    }

    // Success, record the mounted dir
    lstrip(target_path, '/');
    rstrip(target_path, '/');
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
        if (f.is_open()) {
            log::debug("Vfs: success!");
            return f;
        }
    }
    log::debug("Vfs: failed to open file");
    return {};
}


}  // namespace xci::core
