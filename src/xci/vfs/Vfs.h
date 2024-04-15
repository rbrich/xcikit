// Vfs.h created on 2018-09-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2024 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_VFS_H
#define XCI_VFS_H

#include <xci/core/Buffer.h>

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <utility>
#include <array>
#include <filesystem>

namespace xci::vfs {

using namespace xci::core;
namespace fs = std::filesystem;


/// Holds the data loaded from file in memory until released
class VfsFile final {
public:
    /// Create unopened file (eg. when reading failed)
    VfsFile() = default;

    /// Create file object with path and data
    explicit VfsFile(fs::path path, BufferPtr content)
        : m_path(std::move(path)), m_content(std::move(content)) {}

    // move only
    VfsFile(const VfsFile &) = delete;
    VfsFile& operator=(const VfsFile&) = delete;
    VfsFile(VfsFile &&) noexcept = default;
    VfsFile& operator=(VfsFile&&) noexcept = default;

    /// \returns true if file was successfully read
    bool is_open() const { return bool(m_content); }
    bool is_real_file() const { return !m_path.empty(); }

    /// path to file (only regular files, empty for archives)
    const fs::path& path() const { return m_path; }

    /// memory buffer containing the file data
    /// or nullptr if there was error reading the file
    BufferPtr content() const { return m_content; }

    /// View into content, unchecked.
    /// Check content validity before calling this (operator bool or is_open).
    std::string_view content_sv() const { return m_content->string_view(); }

    // convenience operators
    explicit operator bool() const { return is_open(); }

private:
    fs::path m_path;   ///< path of the file in real directory (empty for archives)
    BufferPtr m_content;
};


/// VFS abstract directory.
/// Inherit from this to implement additional archive formats.
/// In case of archives, the "directory" looks up files inside archive by path.
class VfsDirectory: public std::enable_shared_from_this<VfsDirectory> {
public:
    virtual ~VfsDirectory() = default;

    virtual std::string type() const = 0;

    // Read file by name
    virtual VfsFile read_file(const std::string& path) const = 0;

    // List entries, read file by index
    virtual unsigned num_entries() const = 0;
    virtual std::string get_entry_name(unsigned index) const = 0;
    virtual VfsFile read_entry(unsigned index) const = 0;

    class const_iterator {
        friend VfsDirectory;
    public:
        struct Entry {
            friend class const_iterator;
        public:
            std::string name() const { return m_dir.get_entry_name(m_index); }
            VfsFile file() const { return m_dir.read_entry(m_index); }
        private:
            explicit Entry(const VfsDirectory& dir, unsigned index)
                    : m_dir(dir), m_index(index) {}

            const VfsDirectory& m_dir;
            unsigned m_index;
        };

        using difference_type = std::ptrdiff_t;
        using value_type = Entry;
        using iterator_category = std::forward_iterator_tag;

        bool operator==(const const_iterator& rhs) const { return &m_dir == &rhs.m_dir && m_index == rhs.m_index; }

        const_iterator& operator++() {
            ++m_index;
            if (m_index >= m_dir.num_entries())
                m_index = ~0u;
            return *this;
        }
        const_iterator operator++(int) { auto v = *this; ++*this; return v; }

        Entry operator*() const { return Entry(m_dir, m_index); }

    private:
        explicit const_iterator(const VfsDirectory& dir, unsigned index = ~0u)
                : m_dir(dir), m_index(index) {}

        const VfsDirectory& m_dir;
        unsigned m_index;
    };

    const_iterator begin() const    { return const_iterator{*this, 0u}; }
    const_iterator end() const      { return const_iterator{*this}; }
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
    Vfs() : Vfs(Loaders::All) {}
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
    /// - DAR - see `docs/data/archive_format.adoc`
    /// - WAD - DOOM 1 format
    /// - ZIP - zip format via libzip
    ///
    /// The target_path is absolute path inside the VFS (a mount point).
    /// Leading and trailing slashes are ignored, i.e. "" is same as "/",
    /// "path/to" is same as "/path/to/", etc.
    ///
    /// \param fs_path          FS path to a directory or archive.
    /// \param target_path      The target path inside the VFS
    /// \returns true if successfully mounted (archive format identified and supported)
    bool mount(const fs::path& fs_path, std::string target_path="");

    /// Same as above, but instead of mounting real directory or archive file,
    /// mount an archive that is already loaded in memory.
    bool mount_memory(const std::byte* data, size_t size, std::string target_path="");

    /// Read file from VFS.
    /// Tries all mounted dirs/archives that apply for the path, in mounting order.
    /// \param path             The path inside VFS
    /// \returns unopened VfsFile on failure (with bool operator evaluating to false)
    VfsFile read_file(std::string path) const;

    // FIXME: wrap to replace shared_ptr by ref
    struct MountedDir {
        std::string path;  // mounted path
        std::shared_ptr<VfsDirectory> vfs_dir;
    };
    const std::vector<MountedDir>& mounts() const { return m_mounted_dir; }

private:
    // Registered loaders
    std::vector<std::unique_ptr<VfsLoader>> m_loaders;
    // Mounted virtual directories
    std::vector<MountedDir> m_mounted_dir;
};


}  // namespace xci::vfs

#endif // XCI_VFS_H
