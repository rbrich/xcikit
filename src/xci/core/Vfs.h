// Vfs.h created on 2018-09-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_VFS_H
#define XCI_CORE_VFS_H

#include "Buffer.h"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <utility>
#include <array>
#include <filesystem>

namespace xci::core {

namespace fs = std::filesystem;


/// Holds the data loaded from file in memory until released
class VfsFile final {
public:
    // create empty file (eg. when reading failed)
    VfsFile() = default;

    // create file object with path and data
    explicit VfsFile(fs::path path, BufferPtr content)
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
    const fs::path& path() const { return m_path; }

    /// memory buffer containing the file data
    /// or nullptr if there was error reading the file
    BufferPtr content() { return m_content; }

private:
    fs::path m_path;   ///< path of the file or archive containing the file
    BufferPtr m_content;
};


/// VFS abstract directory.
/// Inherit from this to implement additional archive formats.
/// In case of archives, the "directory" looks up files inside archive by path.
class VfsDirectory: public std::enable_shared_from_this<VfsDirectory> {
public:
    virtual ~VfsDirectory() = default;

    virtual VfsFile read_file(const std::string& path) = 0;
};


/// VFS abstract loader.
/// Inherit from this to implement additional archive formats.
/// In case of archives, the loader opens the archive.
class VfsLoader {
public:
    virtual ~VfsLoader() = default;

    /// Name of the loader for logging
    virtual const char* name() const = 0;

    /// Check if loading from the FS directory is supported
    /// \param path     The FS directory to be checked.
    /// \returns        Supported = true.
    virtual bool can_load_fs_dir(const fs::path& path) { return false; }

    /// Load from FS directory
    /// \param path     The FS directory to be loaded.
    /// \returns        Pointer to initialized VfsDirectory or nullptr if failed.
    virtual auto load_fs_dir(const fs::path& path) -> std::shared_ptr<VfsDirectory> { return {}; }

    /// Check if loading from the stream is supported
    /// \param stream   The stream to be checked. The stream is seekable and may not be set to beginning. Call seekg(0) before use.
    /// \returns        Supported = true.
    virtual bool can_load_stream(std::istream& stream) { return false; }

    /// Load from file or memory stream.
    /// \param path     Informative path of original file, only for logging.
    ///                 Do not use for opening the the file, it may contain virtual path like "memory:"
    /// \param stream   Pre-opened file or memory stream.
    /// \returns        Pointer to initialized VfsDirectory or nullptr if failed.
    virtual auto load_stream(std::string&& path, std::unique_ptr<std::istream>&& stream) -> std::shared_ptr<VfsDirectory> { return {}; }
};


namespace vfs {


/// Lookup regular files in real directory, which is mapped to VFS path
class RealDirectoryLoader: public VfsLoader {
public:
    const char* name() const override { return "directory"; };
    bool can_load_fs_dir(const fs::path& path) override { return true; }
    auto load_fs_dir(const fs::path& path) -> std::shared_ptr<VfsDirectory> override;
};

/// Lookup regular files in real directory, which is mapped to VFS path
class RealDirectory: public VfsDirectory {
public:
    explicit RealDirectory(fs::path dir_path) : m_dir_path(std::move(dir_path)) {}

    VfsFile read_file(const std::string& path) override;

private:
    fs::path m_dir_path;
};


/// Lookup files in DAR archive, which is mapped to VFS path
class DarArchiveLoader: public VfsLoader {
public:
    const char* name() const override { return "DAR archive"; };
    bool can_load_stream(std::istream& stream) override;
    auto load_stream(std::string&& path, std::unique_ptr<std::istream>&& stream) -> std::shared_ptr<VfsDirectory> override;
};

/// Lookup files in DAR archive, which is mapped to VFS path
/// DAR is custom uncompressed archive format, see `tools/pack_assets.py`
/// Unlike ZipArchive, this has no external dependency and very simple implementation.
class DarArchive: public VfsDirectory {
public:
    explicit DarArchive(std::string&& path, std::unique_ptr<std::istream>&& stream);
    ~DarArchive() override { close_archive(); }

    bool is_open() const { return bool(m_stream); }

    VfsFile read_file(const std::string& path) override;

private:
    bool read_index(size_t size);
    void close_archive();

private:
    std::string m_path;
    std::unique_ptr<std::istream> m_stream;

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
    bool can_load_stream(std::istream& stream) override;
    auto load_stream(std::string&& path, std::unique_ptr<std::istream>&& stream) -> std::shared_ptr<VfsDirectory> override;
};

/// Lookup files in ZIP archive, which is mapped to VFS path
class ZipArchive: public VfsDirectory {
public:
    explicit ZipArchive(std::string&& path, std::unique_ptr<std::istream>&& stream);
    ~ZipArchive() override;

    bool is_open() const { return m_zip != nullptr; }

    VfsFile read_file(const std::string& path) override;

private:
    std::string m_path;
    std::unique_ptr<std::istream> m_stream;
    void* m_zip = nullptr;
    size_t m_size = 0;
    int m_last_zip_err = 0;
    int m_last_sys_err = 0;
};


}  // namespace vfs


/// Virtual File System
///
/// Search for files by path and open them as file streams.
/// Multiple real FS paths can be mounted to a VFS.
/// When searching for a file, all mounted paths are checked
/// (in order of addition).
/// TODO: One directory can be mounted for writing - any file opened for writing
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
    /// The fs_path doesn't have to exists at the time of addition,
    /// but can be created later. In that case, it will be tried as a directory
    /// every time when opening a file.
    ///
    /// If fs_path is relative (not beginning with '/'), then it's translated
    /// to absolute path using simple lookup, starting with program's location,
    /// then checking its parents (up to hardcoded limit, which is 5 levels
    /// at this time).
    ///
    /// The path can point to an archive instead of a directory.
    /// Supported archive formats:
    /// - DAR - see `tools/pack_assets.py`
    /// - ZIP - when linked with libzip (in cmake: XCI_WITH_ZIP)
    ///
    /// \param fs_path          FS path to a directory or archive.
    /// \param target_path      The target path inside the VFS
    bool mount(const fs::path& fs_path, std::string target_path="");

    /// Same as above, but instead of mounting real directory or archive file,
    /// mount an archive that is already loaded in memory.
    bool mount_memory(const std::byte* data, size_t size, std::string target_path="");

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
