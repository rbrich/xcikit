// Vfs.h created on 2018-09-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
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
    bool is_open() const { return m_content != nullptr; }
    bool is_real_file() const { return !m_path.empty(); }

    /// path to file (only regular files, empty for archives)
    const fs::path& path() const { return m_path; }

    /// memory buffer containing the file data
    /// or nullptr if there was error reading the file
    BufferPtr content() { return m_content; }

    // convenience operators
    explicit operator bool() const { return is_open(); }

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

    virtual unsigned num_entries() const = 0;
    virtual std::string get_entry_name(unsigned index) const = 0;

    class const_iterator {
        friend VfsDirectory;
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = std::string;
        using iterator_category = std::forward_iterator_tag;

        bool operator==(const const_iterator& rhs) const { return &m_dir == &rhs.m_dir && m_index == rhs.m_index; }
        bool operator!=(const const_iterator& rhs) const { return &m_dir != &rhs.m_dir || m_index != rhs.m_index; }

        const_iterator& operator++() {
            ++m_index;
            if (m_index >= m_dir.num_entries())
                m_index = ~0u;
            return *this;
        }
        const_iterator operator++(int) { auto v = *this; ++*this; return v; }

        value_type operator*() const { return m_dir.get_entry_name(m_index); }

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


namespace vfs {


// -------------------------------------------------------------------------------------------------
// Real directory

/// Lookup regular files in real directory, which is mapped to VFS path
class RealDirectoryLoader: public VfsLoader {
public:
    const char* name() const override { return "directory"; }
    bool can_load_fs_dir(const fs::path& path) override { return true; }
    auto load_fs_dir(const fs::path& path) -> std::shared_ptr<VfsDirectory> override;
};

/// Lookup regular files in real directory, which is mapped to VFS path
class RealDirectory: public VfsDirectory {
public:
    explicit RealDirectory(fs::path dir_path) : m_dir_path(std::move(dir_path)) {}

    VfsFile read_file(const std::string& path) override;

    // Calling any of these methods creates a snapshot of the directory,
    // that is used by subsequent calls. Listing live directory would be fragile.
    unsigned num_entries() const override;
    std::string get_entry_name(unsigned index) const override;

private:
    void snapshot_entries() const;

    fs::path m_dir_path;
    mutable std::vector<fs::path> m_entries;  // snapshot of the directory for listing
};


// -------------------------------------------------------------------------------------------------
// DAR archive

/// Lookup files in DAR archive, which is mapped to VFS path
class DarArchiveLoader: public VfsLoader {
public:
    const char* name() const override { return "DAR archive"; }
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
    unsigned num_entries() const override;
    std::string get_entry_name(unsigned index) const override;

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


// -------------------------------------------------------------------------------------------------
// WAD file

/// Lookup files in WAD file (DOOM 1 format), which is mapped to VFS path
class WadArchiveLoader: public VfsLoader {
public:
    const char* name() const override { return "WAD file"; }
    bool can_load_stream(std::istream& stream) override;
    auto load_stream(std::string&& path, std::unique_ptr<std::istream>&& stream) -> std::shared_ptr<VfsDirectory> override;
};

/// Lookup files in WAD file, which is mapped to VFS path
/// WAD is DOOM 1 uncompressed data file format, see `tools/pack_assets.py`
/// Same as DarArchive, this has no external dependency and very simple implementation.
/// Unlike DarArchive, WAD depends on "lump" (archived file) order, lump names can repeat
/// and they are limited to 8 chars.
///
/// This VFS adapter maps the original lump names to virtual paths:
/// * "<lump name>" for normal entries
/// * "_1/<lump name>" for repeated lump names (_1 is the second occurrence, increments for each repetition)
/// * "_MAP01/<lump name>" for map lumps
/// These virtual names has to be used in `get_file`.
/// Alternatively, a program may use directory listing and process the lumps in order.
/// The filename (without subdir) always matches the original lump name.
///
/// Reference: https://doomwiki.org/wiki/WAD
class WadArchive: public VfsDirectory {
    friend class WadArchiveLoader;
public:
    explicit WadArchive(std::string&& path, std::unique_ptr<std::istream>&& stream);
    ~WadArchive() override { close_archive(); }

    bool is_open() const { return bool(m_stream); }

    VfsFile read_file(const std::string& path) override;
    unsigned num_entries() const override;
    std::string get_entry_name(unsigned index) const override;

private:
    bool read_index(size_t size);
    void close_archive();
    VfsFile generate_dot_wad();

    std::string m_path;
    std::unique_ptr<std::istream> m_stream;

    // index:
    struct IndexEntry {
        uint32_t filepos;
        uint32_t size;
        char name[8];  // not zero-terminated, use path() instead
        char subdir[8];  // virtual subdir for the entry, added by loader heuristic

        // Virtual path of the entry. The original lump name is kept as basename,
        // but a subdirectory may be added to uniquely distinguish the entry.
        std::string path() const;
    };
    std::vector<IndexEntry> m_entries;
};


// -------------------------------------------------------------------------------------------------
// ZIP archive

/// Lookup files in ZIP archive, which is mapped to VFS path
class ZipArchiveLoader: public VfsLoader {
public:
    const char* name() const override { return "ZIP archive"; }
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
    unsigned num_entries() const override;
    std::string get_entry_name(unsigned index) const override;

private:
    std::string m_path;
    std::unique_ptr<std::istream> m_stream;
    void* m_zip = nullptr;
    size_t m_size = 0;
    int m_last_zip_err = 0;
    int m_last_sys_err = 0;
};


// -------------------------------------------------------------------------------------------------

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
    /// - DAR - see `tools/pack_assets.py`
    /// - WAD - DOOM 1 format
    /// - ZIP - when linked with libzip (in cmake: XCI_WITH_ZIP)
    ///
    /// \param fs_path          FS path to a directory or archive.
    /// \param target_path      The target path inside the VFS
    /// \returns true if successfully mounted (archive format identified and supported)
    bool mount(const fs::path& fs_path, std::string target_path="");

    /// Same as above, but instead of mounting real directory or archive file,
    /// mount an archive that is already loaded in memory.
    bool mount_memory(const std::byte* data, size_t size, std::string target_path="");

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


}  // namespace xci::core

#endif // XCI_CORE_VFS_H
