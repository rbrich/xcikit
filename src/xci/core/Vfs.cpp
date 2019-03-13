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
#include <xci/compat/bit.h>
#include <xci/compat/endian.h>
#include <xci/compat/macros.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <algorithm>

namespace xci::core {

using namespace core::log;
using xci::core::bit_read;


std::shared_ptr<VfsDirectory> vfs::RealDirectoryLoader::try_load(const std::string& path)
{
    struct stat st = {};
    auto rc = ::stat(path.c_str(), &st);
    if (rc != 0 || (st.st_mode & S_IFDIR) != S_IFDIR)
        return {};
    return std::make_shared<vfs::RealDirectory>(path);
}


VfsFile vfs::RealDirectory::read_file(const std::string& path)
{
    auto full_path = path_join(m_dir_path, path);
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


static constexpr char c_dar_magic[] = "dar\n";


std::shared_ptr<VfsDirectory> vfs::DarArchiveLoader::try_load(const std::string& path)
{
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd == -1)
        return {};
    std::string buf(4, ' ');
    if (::read(fd, &buf[0], 4) != 4 || buf != c_dar_magic)
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


VfsFile vfs::DarArchive::read_file(const std::string& path)
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
            [this_ptr](byte* d, size_t s) {});
    return VfsFile("", std::move(content));
}


bool vfs::DarArchive::read_index()
{
    // HEADER: ID
    if (std::string(reinterpret_cast<char*>(m_addr), 4) != c_dar_magic) {
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
    auto addr = m_addr + index_offset;
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


Vfs& Vfs::default_instance()
{
    static Vfs instance(Loaders::All);
    return instance;
}


Vfs::Vfs(Loaders loaders)
{
    switch (loaders) {
        case Loaders::All:
            m_loaders.emplace_back(std::make_unique<vfs::DarArchiveLoader>());
            FALLTHROUGH;
        case Loaders::RealDirectory:
            m_loaders.emplace_back(std::make_unique<vfs::RealDirectoryLoader>());
            FALLTHROUGH;
        case Loaders::None:
            break;
    }
}


bool Vfs::mount(std::string real_path, std::string target_path)
{
    std::shared_ptr<VfsDirectory> vfs_directory;
    for (auto& loader : m_loaders) {
        vfs_directory = loader->try_load(real_path);
        if (!vfs_directory)
            continue;
        log_info("Vfs: mount {} ({})", real_path, loader->name());
        break;
    }
    if (!vfs_directory) {
        log_warning("Vfs: couldn't mount {}", real_path);
        return false;
    }
    lstrip(target_path, '/');
    rstrip(target_path, '/');
    m_mounted_dir.push_back({std::move(target_path), std::move(vfs_directory)});
    return true;
}


VfsFile Vfs::read_file(std::string path)
{
    lstrip(path, '/');
    log_debug("Vfs: try open: {}", path);
    for (auto& path_loader : m_mounted_dir) {
        // Is the loader applicable for requested path?
        if (!path_loader.path.empty()) {
            if (!starts_with(path, path_loader.path))
                continue;
            path = path.substr(path_loader.path.size());
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
