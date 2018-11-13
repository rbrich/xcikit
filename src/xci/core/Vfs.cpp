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
#include <xci/core/bit_read.h>
#include <xci/compat/endian.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

namespace xci::core {

using namespace core::log;
using xci::core::bit_read;


BufferPtr VfsFile::content()
{
    if (!m_content)
        m_content = read_binary_file(m_fstream);
    return m_content;
}


bool VfsDirLoader::can_handle(const std::string& path)
{
    struct stat st = {};
    auto rc = ::stat(path.c_str(), &st);
    return rc == 0 && (st.st_mode & S_IFDIR) == S_IFDIR;
}


VfsFile VfsDirLoader::open(const std::string& path, std::ios_base::openmode mode)
{
    auto full_path = path_join(m_path, path);
    log_debug("VfsDirLoader: open file: {}", full_path);
    return VfsFile(std::move(full_path), mode);
}


static constexpr char c_dar_magic[] = "dar\n";


VfsDarArchiveLoader::VfsDarArchiveLoader(std::string path)
{
    // open archive file
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        log_error("VfsDarArchiveLoader: Failed to open file: {}: {m}", path.c_str());
        return;
    }

    // obtain file size
    struct stat st = {};
    if (::fstat(fd, &st) == -1) {
        log_error("VfsDarArchiveLoader: Failed to stat file: {}: {m}", path.c_str());
        ::close(fd);
        return;
    }
    m_size = (size_t)(st.st_size);

    // map whole archive into memory
    m_addr = (Byte*)::mmap(nullptr, m_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (m_addr == MAP_FAILED) {
        log_error("VfsDarArchiveLoader: Failed to mmap file: {}: {m}", path.c_str());
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


bool VfsDarArchiveLoader::can_handle(const std::string& path)
{
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd == -1)
        return false;
    std::string buf(4, ' ');
    if (::read(fd, &buf[0], 4) != 4)
        return false;
    return buf == c_dar_magic;
}


VfsFile VfsDarArchiveLoader::open(const std::string& path, std::ios_base::openmode mode)
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

    // FIXME: mmap just the one file here and pass deleter with munmap
    return VfsFile(m_addr + entry_it->offset, entry_it->size);
}


bool VfsDarArchiveLoader::read_index()
{
    // HEADER: ID
    if (std::string(reinterpret_cast<char*>(m_addr), 4) != c_dar_magic) {
        log_error("VfsDarArchiveLoader: Corrupted archive ({}).", "ID");
        return false;
    }
    // HEADER: INDEX_OFFSET
    auto index_offset = be32toh(bit_read<uint32_t>(m_addr + 4));
    if (index_offset + 4 > m_size) {
        // the offset must be inside archive, plus 4B for num_entries
        log_error("VfsDarArchiveLoader: Corrupted archive ({}).", "INDEX_OFFSET");
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
            log_error("VfsDarArchiveLoader: Corrupted archive ({}).", "INDEX_ENTRY");
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
            log_error("VfsDarArchiveLoader: Corrupted archive ({}).",
                    "CONTENT_OFFSET + CONTENT_SIZE");
            return false;
        }
        // INDEX_ENTRY: NAME_SIZE
        auto name_size = be16toh(bit_read<uint16_t>(addr));
        addr += 2;
        // INDEX_ENTRY: NAME
        if ((size_t)(addr - m_addr) + name_size > m_size) {
            // there must be space for the name in archive
            log_error("VfsDarArchiveLoader: Corrupted archive ({}).", "NAME");
            return false;
        }
        entry.name.assign(reinterpret_cast<char*>(addr), name_size);
        addr += name_size;
    }
    return true;
}


void VfsDarArchiveLoader::close_archive()
{
    if (m_addr != nullptr && m_addr != MAP_FAILED)
        ::munmap(m_addr, m_size);
    m_addr = nullptr;
    m_size = 0;
}


Vfs& Vfs::default_instance()
{
    static Vfs instance;
    return instance;
}


bool Vfs::mount(std::string real_path, std::string target_path)
{
    std::unique_ptr<VfsLoader> loader;
    if (VfsDirLoader::can_handle(real_path)) {
        loader = std::make_unique<VfsDirLoader>(std::move(real_path));
        log_info("Vfs: mounted {} (directory)", real_path);
    }
    else if (VfsDarArchiveLoader::can_handle(real_path)) {
        loader = std::make_unique<VfsDarArchiveLoader>(std::move(real_path));
        log_info("Vfs: mounted {} (DAR archive)", real_path);
    }
    else {
        log_warning("Vfs: couldn't mount {}", real_path);
        return false;
    }
    lstrip(target_path, '/');
    rstrip(target_path, '/');
    m_mounted_loaders.push_back({std::move(target_path), std::move(loader)});
    return true;
}


VfsFile Vfs::open(std::string path, std::ios_base::openmode mode)
{
    lstrip(path, '/');
    log_debug("Vfs: try open: {}", path);
    for (auto& path_loader : m_mounted_loaders) {
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
        auto f = path_loader.loader->open(path, mode);
        if (f.is_open()) {
            log_debug("Vfs: success!");
            return f;
        }
    }
    log_debug("Vfs: failed to open file");
    return {};
}


}  // namespace xci::core
