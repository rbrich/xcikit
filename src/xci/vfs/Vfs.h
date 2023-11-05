// Vfs.h created on 2018-09-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_VFS_H
#define XCI_CORE_VFS_H

#include <xci/core/Buffer.h>

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
    BufferPtr content() const { return m_content; }

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
        bool operator!=(const const_iterator& rhs) const { return &m_dir != &rhs.m_dir || m_index != rhs.m_index; }

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

    std::string type() const override { return "DIR"; }

    VfsFile read_file(const std::string& path) const override;

    // Calling any of these methods creates a snapshot of the directory,
    // that is used by subsequent calls. Listing live directory would be fragile.
    unsigned num_entries() const override;
    std::string get_entry_name(unsigned index) const override;
    VfsFile read_entry(unsigned index) const override;

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

    std::string type() const override { return "DAR"; }

    bool is_open() const { return bool(m_stream); }

    VfsFile read_file(const std::string& path) const override;
    unsigned num_entries() const override;
    std::string get_entry_name(unsigned index) const override;
    VfsFile read_entry(unsigned index) const override;

private:
    struct IndexEntry {
        uint32_t offset;
        uint32_t size;
        std::string name;
    };

    bool read_index(size_t size);
    VfsFile read_entry(const IndexEntry& entry) const;
    void close_archive();

    std::string m_path;
    std::unique_ptr<std::istream> m_stream;

    // index:
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
/// WAD is DOOM 1 uncompressed data file format.
/// Same as DarArchive, this has no external dependency and very simple implementation.
/// Unlike DarArchive, WAD depends on lump order (lump = archived file), lump names can repeat
/// and they are limited to 8 chars.
/// When looking up files by name, only the first lump of that name is returned.
/// Use entry listing to process lumps in order.
/// Reference: https://doomwiki.org/wiki/WAD
class WadArchive: public VfsDirectory {
    friend class WadArchiveLoader;
public:
    explicit WadArchive(std::string&& path, std::unique_ptr<std::istream>&& stream);
    ~WadArchive() override { close_archive(); }

    std::string type() const override;  // "IWAD" or "PWAD"
    bool is_open() const { return bool(m_stream); }

    VfsFile read_file(const std::string& path) const override;
    unsigned num_entries() const override;
    std::string get_entry_name(unsigned index) const override;
    VfsFile read_entry(unsigned index) const override;

private:
    struct IndexEntry {
        uint32_t filepos;
        uint32_t size;
        char name[8];  // not zero-terminated, use path() instead

        std::string path() const;
    };
    static_assert(sizeof(IndexEntry) == 16);

    bool read_index(size_t size);
    VfsFile read_entry(const IndexEntry& entry) const;
    void close_archive();

    std::string m_path;
    std::unique_ptr<std::istream> m_stream;

    // index:
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

    std::string type() const override { return "ZIP"; }
    bool is_open() const { return m_zip != nullptr; }

    VfsFile read_file(const std::string& path) const override;
    unsigned num_entries() const override;
    std::string get_entry_name(unsigned index) const override;
    VfsFile read_entry(unsigned index) const override;

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
    /// - ZIP - zip format via libzip
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
