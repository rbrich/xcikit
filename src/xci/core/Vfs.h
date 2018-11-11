#include <utility>

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

#include <string>
#include <vector>
#include <memory>
#include <fstream>

namespace xci::core {


class VfsFile : public std::iostream {
public:
    VfsFile() : std::iostream(&m_filebuf) {}
    explicit VfsFile(std::string path,
                     std::ios_base::openmode mode = std::ios_base::in)
        : VfsFile() { open(std::move(path), mode); }

    // move/swap
    VfsFile(const VfsFile&) = delete;
    VfsFile(VfsFile&& rhs): std::iostream(std::move(rhs)),
                            m_filebuf(std::move(rhs.m_filebuf)),
                            m_path(std::move(rhs.m_path))
    { std::iostream::set_rdbuf(&m_filebuf); }
    VfsFile& operator=(const VfsFile&) = delete;
    VfsFile& operator=(VfsFile&& rhs) {
        std::iostream::operator=(std::move(rhs));
        m_filebuf = std::move(rhs.m_filebuf);
        m_path = std::move(rhs.m_path);
        return *this;
    }
    void swap(VfsFile& rhs) {
        std::iostream::swap(rhs);
        m_filebuf.swap(rhs.m_filebuf);
        m_path.swap(rhs.m_path);
    }

    // emulate std::fstream
    bool is_open() const { return m_filebuf.is_open(); }
    void open(std::string path,
              std::ios_base::openmode mode = std::ios_base::in) {
        m_path = std::move(path);
        if (m_filebuf.open(m_path, mode))
            clear();
        else
            setstate(ios_base::failbit);
    }
    void close() {
        if (!m_filebuf.close())
            setstate(ios_base::failbit);
    }

    // publish real path info when available
    bool is_real_file() const { return true; }
    const std::string& path() const { return m_path; }

private:
    std::filebuf m_filebuf;
    std::string m_path;
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

    VfsFile open(const std::string& path, std::ios_base::openmode mode) override;

private:
    std::string m_path;
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
    void mount(std::string real_path, std::string target_path="");

    VfsFile open(std::string path,
                 std::ios_base::openmode mode = std::ios_base::in);

private:
    struct PathLoader {
        std::string path;  // mounted path
        std::unique_ptr<VfsLoader> loader;
    };
    std::vector<PathLoader> m_loaders;
};


}  // namespace xci::core

#endif // XCI_CORE_VFS_H
