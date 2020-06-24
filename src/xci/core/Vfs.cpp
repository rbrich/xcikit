// Vfs.cpp created on 2018-09-01, part of XCI toolkit
// Copyright 2018 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Vfs.h"
#include "file.h"
#include <xci/core/log.h>
#include <xci/core/string.h>
#include <xci/core/sys.h>
#include <xci/core/file.h>
#include <xci/compat/bit.h>
#include <xci/compat/endian.h>
#include <xci/compat/macros.h>
#include <xci/compat/unistd.h>
#include <xci/compat/mman.h>

#ifdef XCI_WITH_ZIP
#include <zip.h>
#endif

#include <algorithm>
#include <fcntl.h>
#include <sys/stat.h>

namespace xci::core {

using namespace core::log;
using xci::bit_read;


std::shared_ptr<VfsDirectory>
vfs::RealDirectoryLoader::try_load(const std::string& path, bool is_dir, Magic)
{
    if (!is_dir)
        return {};
    return std::make_shared<vfs::RealDirectory>(path);
}


VfsFile vfs::RealDirectory::read_file(const std::string& path) const
{
    auto full_path = path::join(m_dir_path, path);
    log_debug("VfsDirLoader: open file: {}", full_path);

    // open the file
    int fd = ::open(full_path.c_str(), O_RDONLY);
    if (fd == -1) {
        log_error("VfsDirLoader: Failed to open file: {}: {m}", full_path.c_str());
        return {};
    }

    // obtain file size
    struct stat st = {};
    if (::fstat(fd, &st) == -1) {
        log_error("VfsDirLoader: Failed to stat file: {}: {m}", full_path.c_str());
        ::close(fd);
        return {};
    }
    auto size = static_cast<size_t>(st.st_size);

    // map file into memory
    void* addr = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        log_error("VfsDirLoader: Failed to mmap file: {}: {m}", full_path.c_str());
        ::close(fd);
        return {};
    }

    // close file (it's already mapped, so we no longer need FD)
    ::close(fd);

    auto content = std::make_shared<Buffer>(
            static_cast<byte*>(addr), size,
            [](byte* d, size_t s) { ::munmap(d, s); });

    return VfsFile(std::move(full_path), std::move(content));
}


// ----------------------------------------------------------------------------


static constexpr std::array<char, 4> c_dar_magic = {{'d', 'a', 'r', '\n'}};


std::shared_ptr<VfsDirectory>
vfs::DarArchiveLoader::try_load(const std::string& path, bool is_dir, Magic magic)
{
    if (is_dir || magic != c_dar_magic)
        return {};
    return std::make_shared<vfs::DarArchive>(path);
}


vfs::DarArchive::DarArchive(std::string path)
    : m_archive_path(std::move(path))
{
    TRACE("Opening archive: {}", m_archive_path);

    // open archive file
    int fd = ::open(m_archive_path.c_str(), O_RDONLY);
    if (fd == -1) {
        log_error("VfsDarArchiveLoader: Failed to open file: {}: {m}", m_archive_path);
        return;
    }

    // obtain file size
    struct stat st = {};
    if (::fstat(fd, &st) == -1) {
        log_error("VfsDarArchiveLoader: Failed to stat file: {}: {m}", m_archive_path);
        ::close(fd);
        return;
    }
    m_size = (size_t)(st.st_size);

    // map whole archive into memory
    m_addr = (byte*)::mmap(nullptr, m_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (m_addr == MAP_FAILED) {
        log_error("VfsDarArchiveLoader: Failed to mmap file: {}: {m}", m_archive_path);
        ::close(fd);
        close_archive();
        return;
    }

    // close file (it's already mapped, so we no longer need FD)
    ::close(fd);

    // read archive index
    if (!read_index()) {
        close_archive();
    }
}


VfsFile vfs::DarArchive::read_file(const std::string& path) const
{
    // search for the entry
    auto entry_it = std::find_if(m_entries.cbegin(), m_entries.cend(), [&path](auto& entry){
        return entry.name == path;
    });
    if (entry_it == m_entries.cend()) {
        log_error("VfsDarArchiveLoader: Not found in archive: {}", path);
        return {};
    }

    // return a view into mmapped archive
    log_debug("VfsDarArchiveLoader: open file: {}", path);

    // Pass self to Buffer deleter, so the archive object lives
    // at least as long as the buffer.
    auto this_ptr = shared_from_this();
    auto content = std::make_shared<Buffer>(
            m_addr + entry_it->offset, entry_it->size,
            [this_ptr](byte*, size_t) {});
    return VfsFile("", std::move(content));
}


bool vfs::DarArchive::read_index()
{
    // HEADER: ID
    if (::memcmp(m_addr, c_dar_magic.data(), c_dar_magic.size()) != 0) {
        log_error("VfsDarArchiveLoader: Corrupted archive: {} ({}).",
                  m_archive_path, "ID");
        return false;
    }
    // HEADER: INDEX_OFFSET
    auto index_offset = be32toh(bit_read<uint32_t>(m_addr + 4));
    if (index_offset + 4 > m_size) {
        // the offset must be inside archive, plus 4B for num_entries
        log_error("VfsDarArchiveLoader: Corrupted archive: {} ({}).",
                  m_archive_path, "INDEX_OFFSET");
        return false;
    }
    // INDEX: NUMBER_OF_ENTRIES
    auto* addr = m_addr + index_offset;
    auto num_entries = be32toh(bit_read<uint32_t>(addr));
    addr += 4;
    // INDEX: INDEX_ENTRY[]
    m_entries.resize(num_entries);
    for (unsigned i = 0; i < num_entries; i++) {
        if ((size_t)(addr - m_addr) + 10 > m_size) {
            // there must be space for the entry (4+4+2 bytes)
            log_error("VfsDarArchiveLoader: Corrupted archive: {} ({}).",
                      m_archive_path, "INDEX_ENTRY");
            return false;
        }
        auto& entry = m_entries[i];
        // INDEX_ENTRY: CONTENT_OFFSET
        entry.offset = be32toh(bit_read<uint32_t>(addr));
        addr += 4;
        // INDEX_ENTRY: CONTENT_SIZE
        entry.size = be32toh(bit_read<uint32_t>(addr));
        addr += 4;
        if (entry.offset + entry.size > index_offset) {
            // there must be space for the name in archive
            log_error("VfsDarArchiveLoader: Corrupted archive: {} ({}).",
                      m_archive_path, "CONTENT_OFFSET + CONTENT_SIZE");
            return false;
        }
        // INDEX_ENTRY: NAME_SIZE
        auto name_size = be16toh(bit_read<uint16_t>(addr));
        addr += 2;
        // INDEX_ENTRY: NAME
        if ((size_t)(addr - m_addr) + name_size > m_size) {
            // there must be space for the name in archive
            log_error("VfsDarArchiveLoader: Corrupted archive: {} ({}).",
                      m_archive_path, "NAME");
            return false;
        }
        entry.name.assign(reinterpret_cast<char*>(addr), name_size);
        addr += name_size;
    }
    return true;
}


void vfs::DarArchive::close_archive()
{
    if (m_addr != nullptr && m_addr != MAP_FAILED) {
        TRACE("Closing archive: {}", m_archive_path);
        ::munmap(m_addr, m_size);
    }
    m_addr = nullptr;
    m_size = 0;
}


// ----------------------------------------------------------------------------


std::shared_ptr<VfsDirectory>
vfs::ZipArchiveLoader::try_load(const std::string& path, bool is_dir, Magic magic)
{
    if (is_dir || magic[0] != 'P' || magic[1] != 'K')
        return {};

    auto zip = std::make_shared<vfs::ZipArchive>(path);
    if (!zip->is_open())
        return {};
    return zip;
}


vfs::ZipArchive::ZipArchive(std::string path)
    : m_zip_path(std::move(path))
{
    TRACE("ZipArchive: Opening archive: {}", m_zip_path);
#ifdef XCI_WITH_ZIP
    // open archive file
    int err = 0;
    zip_t* f = zip_open(m_zip_path.c_str(), ZIP_RDONLY, &err);
    if (f == nullptr) {
        log_error("ZipArchive: Failed to open archive: {}: {}", m_zip_path, err);
        return;
    }

    m_zip = f;
#else
    log_error("ZipArchive: Not supported (not compiled with XCI_WITH_ZIP)");
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


VfsFile vfs::ZipArchive::read_file(const std::string& path) const
{
#ifdef XCI_WITH_ZIP
    struct zip_stat st = {};
    zip_stat_init(&st);
    if (zip_stat((zip_t*) m_zip, path.c_str(), ZIP_FL_ENC_RAW, &st) == -1) {
        auto err = zip_get_error((zip_t*) m_zip);
        if (err->zip_err == ZIP_ER_NOENT) {
            log_error("ZipArchive: Not found in archive: {}", path);
        } else {
            log_error("ZipArchive: Cannot read: {}: {}", path, err->str);
        }
        return {};
    }
    if ((st.valid & ZIP_STAT_SIZE) != ZIP_STAT_SIZE) {
        log_error("ZipArchive: Cannot read: {} (missing size?!)", path);
        return {};
    }

    byte* data = new byte[st.size];

    zip_file* f = zip_fopen_index((zip_t*) m_zip, st.index, 0);
    zip_int64_t nbytes = zip_fread(f, data, st.size);
    zip_fclose(f);
    if (nbytes < 0 || (zip_uint64_t)nbytes != st.size) {
        log_error("ZipArchive: Cannot read: {}: Read {} bytes of {}",
                path, nbytes, st.size);
        delete[] data;
        return {};
    }

    auto content = std::make_shared<Buffer>(
        data, (size_t) st.size,
        /*deleter*/ [](byte* d, size_t s) { delete[] d; });
    return VfsFile("", std::move(content));
#else
    UNUSED path;
    log_error("ZipArchive: Not supported (not compiled with XCI_WITH_ZIP)");
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


bool Vfs::mount(const std::string& fs_path, std::string target_path)
{
    std::string real_path;
    if (fs_path.empty() || fs_path[0] != '/') {
        // handle relative path - it's relative to program binary,
        // or its parent, or its parent's parent. The nearest matched parent wins.
        std::string ups = "/";
        for (int parent = 0; parent < 5; ++parent) {
            real_path = path::real_path(path::dir_name(get_self_path()) + ups + fs_path);
            if (!real_path.empty())
                break;
            ups += "../";
        }
    } else {
        real_path = fs_path;
    }

    // Check path type
    struct stat st = {};
    if (::stat(real_path.c_str(), &st) == -1) {
        log_warning("Vfs: couldn't mount {}: {m}", real_path);
        return false;
    }

    // Read magic bytes in case of regular file
    VfsLoader::Magic magic {};
    if ((st.st_mode & S_IFREG) == S_IFREG) {
        int fd = ::open(real_path.c_str(), O_RDONLY);
        if (fd == -1) {
            log_warning("Vfs: couldn't mount {}: {m}", real_path);
            return false;
        }
        auto r = ::read(fd, magic.data(), magic.size());
        ::close(fd);
        if (r != (ssize_t) magic.size()) {
            log_warning("Vfs: couldn't mount {}: archive file is smaller than {} bytes",
                real_path, magic.size());
            return false;
        }
    }

    // Try each loader
    bool is_dir = (st.st_mode & S_IFDIR) == S_IFDIR;
    std::shared_ptr<VfsDirectory> vfs_directory;
    for (auto& loader : m_loaders) {
        vfs_directory = loader->try_load(real_path, is_dir, magic);
        if (!vfs_directory)
            continue;
        log_info("Vfs: mount {} ({})", real_path, loader->name());
        break;
    }
    if (!vfs_directory) {
        log_warning("Vfs: couldn't mount {}", real_path);
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
    log_debug("Vfs: try open: {}", path);
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
            log_debug("Vfs: success!");
            return f;
        }
    }
    log_debug("Vfs: failed to open file");
    return {};
}


}  // namespace xci::core
