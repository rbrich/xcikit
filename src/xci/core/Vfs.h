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

#include "Buffer.h"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <utility>
#include <array>

namespace xci::core {


/// Holds the data loaded from file in memory until released
class VfsFile final {
public:
    // create empty file (eg. when reading failed)
    VfsFile() = default;

    // create file object with path and data
    explicit VfsFile(std::string path, BufferPtr content)
        : m_path(std::move(path)), m_content(std::move(content)) {}

    // move only
    VfsFile(const VfsFile &) = delete;
    VfsFile& operator=(const VfsFile&) = delete;
    VfsFile(VfsFile &&) noexcept = default;
    VfsFile& operator=(VfsFile&&) noexcept = default;

    /// \returns true if file was successfully read
    bool is_open() { return m_content != nullptr; }
    bool is_real_file() { return !m_path.empty(); }

    /// path to file (only regular files, empty for archives)
    const std::string& path() const { return m_path; }

    /// memory buffer containing the file data
    /// or nullptr if there was error reading the file
    BufferPtr content() { return m_content; }

private:
    std::string m_path;   ///< path of the file or archive containing the file
    BufferPtr m_content;
};


/// VFS abstract directory.
/// Inherit from this to implement additional archive formats.
/// In case of archives, the "directory" looks up files inside archive by path.
class VfsDirectory: public std::enable_shared_from_this<VfsDirectory> {
public:
    virtual ~VfsDirectory() = default;

    virtual VfsFile read_file(const std::string& path) const = 0;
};


/// VFS abstract loader.
/// Inherit from this to implement additional archive formats.
/// In case of archives, the loader opens the archive.
class VfsLoader {
public:
    virtual ~VfsLoader() = default;

    using Magic = std::array<char, 4>;

    virtual const char* name() const = 0;
    virtual std::shared_ptr<VfsDirectory>
        try_load(const std::string& path, bool is_dir, Magic magic) = 0;
};


namespace vfs {


/// Lookup regular files in real directory, which is mapped to VFS path
class RealDirectoryLoader: public VfsLoader {
public:
    const char* name() const override { return "directory"; };
    std::shared_ptr<VfsDirectory>
        try_load(const std::string& path, bool is_dir, Magic magic) override;
};

/// Lookup regular files in real directory, which is mapped to VFS path
class RealDirectory: public VfsDirectory {
public:
    explicit RealDirectory(std::string dir_path) : m_dir_path(std::move(dir_path)) {}

    VfsFile read_file(const std::string& path) const override;

private:
    std::string m_dir_path;
};


/// Lookup files in DAR archive, which is mapped to VFS path
class DarArchiveLoader: public VfsLoader {
public:
    const char* name() const override { return "DAR archive"; };
    std::shared_ptr<VfsDirectory>
        try_load(const std::string& path, bool is_dir, Magic magic) override;
};

/// Lookup files in DAR archive, which is mapped to VFS path
/// DAR is custom uncompressed archive format, see `tools/pack_assets.py`
/// Unlike ZipArchive, this has no external dependency and very simple implementation.
class DarArchive: public VfsDirectory {
public:
    explicit DarArchive(std::string path);
    ~DarArchive() override { close_archive(); }

    VfsFile read_file(const std::string& path) const override;

private:
    bool read_index();
    void close_archive();

private:
    std::string m_archive_path;

    // mmapped archive:
    byte* m_addr = nullptr;
    size_t m_size = 0;

    // index:
    struct IndexEntry {
        uint32_t offset;
        uint32_t size;
        std::string name;
    };
    std::vector<IndexEntry> m_entries;
};


/// Lookup files in ZIP archive, which is mapped to VFS path
class ZipArchiveLoader: public VfsLoader {
public:
    const char* name() const override { return "ZIP archive"; };
    std::shared_ptr<VfsDirectory>
        try_load(const std::string& path, bool is_dir, Magic magic) override;
};

/// Lookup files in ZIP archive, which is mapped to VFS path
class ZipArchive: public VfsDirectory {
public:
    explicit ZipArchive(std::string path);
    ~ZipArchive() override;

    bool is_open() const { return m_zip != nullptr; }

    VfsFile read_file(const std::string& path) const override;

private:
    std::string m_zip_path;
    void* m_zip = nullptr;
};


}  // namespace vfs


/// Virtual File System
///
/// Search for files by path and open them as file streams.
/// Multiple real FS paths can be mounted to a VFS.
/// When searching for a file, all mounted paths are checked
/// (in order of addition).
/// TODO: Single dir can be mounted for writing - any file opened for writing
///       will be created in that dir.
class Vfs {
public:
    enum class Loaders {
        None,               // do not preload any loaders
        NoArchives,         // preload RealDirectoryLoader
        NoZip,              // preload RealDirectoryLoader, DarArchiveLoader
        All,                // preload all loaders (incl. ZipArchiveLoader when available)
    };
    Vfs() : Vfs(Loaders::All) {};
    explicit Vfs(Loaders loaders);

    /// Register custom loader
    void add_loader(std::unique_ptr<VfsLoader> loader) { m_loaders.emplace_back(std::move(loader)); }

    /// Mount real FS path to a VFS path.
    ///
    /// Multiple dirs can overlap - they will be searched in order of addition.
    ///
    /// The fs_path doesn't have to exists at time of addition,
    /// but can be created later. In that case, it will be tried as a directory
    /// every time when opening a file.
    ///
    /// If fs_path is relative (not begining with '/'), then it's translated
    /// to absolute path using simple lookup, starting with program's location,
    /// then checking its parents (up to hardcoded limit, which is 5 levels
    /// at this time).
    ///
    /// The path can point to an archive instead of directory.
    /// Supported archive formats:
    /// - DAR - see `tools/pack_assets.py`
    /// - ZIP - when linked with libzip (in cmake: XCI_WITH_ZIP)
    ///
    /// \param fs_path          FS path to a directory or archive.
    /// \param target_path      The target path inside the VFS
    bool mount(const std::string& fs_path, std::string target_path="");

    VfsFile read_file(std::string path) const;

private:
    // Registered loaders
    std::vector<std::unique_ptr<VfsLoader>> m_loaders;
    // Mounted virtual directories
    struct MountedDir {
        std::string path;  // mounted path
        std::shared_ptr<VfsDirectory> vfs_dir;
    };
    std::vector<MountedDir> m_mounted_dir;
};


}  // namespace xci::core

#endif // XCI_CORE_VFS_H
