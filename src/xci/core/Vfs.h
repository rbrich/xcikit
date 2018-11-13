// Vfs.h created on 2018-09-01, part of XCI toolkit
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

#ifndef XCI_CORE_VFS_H
#define XCI_CORE_VFS_H

#include "types.h"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <utility>

namespace xci::core {


class VfsFile {
public:
    VfsFile() = default;
    explicit VfsFile(std::string path,
                     std::ios_base::openmode mode = std::ios_base::in)
        : m_fstream(path, mode), m_path(std::move(path)) {}

    explicit VfsFile(Byte* data, std::size_t size)
        : m_content(new Buffer(data, size)) {}

    template<class TDeleter>
    explicit VfsFile(Byte* data, std::size_t size, TDeleter d)
        : m_content(new Buffer(data, size), d) {}

    // check
    bool is_open() { return m_fstream.is_open() || m_content != nullptr; }

    // publish real path info when available
    bool is_real_file() const { return !m_path.empty(); }
    const std::string& path() const { return m_path; }
    std::iostream& stream() { return m_fstream; }

    // publish data
    BufferPtr content();

private:
    std::fstream m_fstream;
    std::string m_path;
    BufferPtr m_content;
};


/// VFS abstract loader.
/// Inherit from this to implement additional archive format.
class VfsLoader {
public:
    virtual ~VfsLoader() = default;
    virtual VfsFile open(const std::string& path, std::ios_base::openmode mode) = 0;
};


/// Lookup files in real directory, which is mapped to VFS path
class VfsDirLoader: public VfsLoader {
public:
    explicit VfsDirLoader(std::string m_path) : m_path(std::move(m_path)) {}

    static bool can_handle(const std::string& path);

    VfsFile open(const std::string& path, std::ios_base::openmode mode) override;

private:
    std::string m_path;
};


/// Lookup files in real directory, which is mapped to VFS path
class VfsDarArchiveLoader: public VfsLoader {
public:
    explicit VfsDarArchiveLoader(std::string path);

    static bool can_handle(const std::string& path);

    VfsFile open(const std::string& path, std::ios_base::openmode mode) override;

private:
    // mmapped archive:
    Byte* m_addr = nullptr;
    size_t m_size = 0;

    // index:
    struct IndexEntry {
        uint32_t offset;
        uint32_t size;
        std::string name;
    };
    std::vector<IndexEntry> m_entries;

    bool read_index();
    void close_archive();
};


/// Virtual File System
///
/// Search for files by path and open them as file streams.
/// Multiple real FS paths can be mounted as to a VFS.
/// When searching for a file, all mounted paths are checked
/// (in order of addition).
/// TODO: Single dir can be mounted for writing - any file opened for writing
///       will be created in that dir.
class Vfs {
public:
    static Vfs& default_instance();

    /// Mount real FS path to a VFS path.
    ///
    /// Multiple dirs can overlap - they will be searched in order of addition.
    ///
    /// The real_path doesn't have to exists at time of addition,
    /// but can be created later. In that case, it will be tried as a directory
    /// every time when opening a file.
    ///
    /// The path can point to an archive instead of directory.
    /// Supported archive formats:
    /// - DAR - see `tools/pack_assets.py`
    /// - ZIP - TODO
    ///
    /// \param real_path        FS path to a directory or archive.
    /// \param target_path      The target path inside the VFS
    bool mount(std::string real_path, std::string target_path="");

    VfsFile open(std::string path,
                 std::ios_base::openmode mode = std::ios_base::in);

private:
    struct MountedLoader {
        std::string path;  // mounted path
        std::unique_ptr<VfsLoader> loader;
    };
    std::vector<MountedLoader> m_mounted_loaders;

};


}  // namespace xci::core

#endif // XCI_CORE_VFS_H
