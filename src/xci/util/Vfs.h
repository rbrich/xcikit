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

#ifndef XCI_UTIL_VFS_H
#define XCI_UTIL_VFS_H

#include <string>
#include <vector>
#include <memory>
#include <fstream>

namespace xci::util {


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


class VfsLoader {
public:
    virtual VfsFile open(const std::string& path, std::ios_base::openmode mode) = 0;
};

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
/// Multiple real FS paths can be mounted as a root of VFS.
/// When searching for a file, all mounted paths are checked
/// (in order of addition).
/// TODO: Single dir can be mounted for writing - any file opened for writing
///       will be created in that dir.
class Vfs {
public:
    static Vfs& default_instance();

    /// Mount real FS dir as root of the VFS.
    /// Multiple dirs can be added this way - they will be searched
    /// in order of addition. The path don't have to exists at time of addition,
    /// but can be created later. It will be checked every time when opening a file.
    void mount_dir(std::string path);

    // TODO: return custom stream class inherited from `basic_iostream`
    //       (to make it possible to abstract also zip files etc.)
    VfsFile open(const std::string& path,
                 std::ios_base::openmode mode = std::ios_base::in);

private:
    std::vector<std::unique_ptr<VfsLoader>> m_loaders;
};


}  // namespace xci::util

#endif // XCI_UTIL_VFS_H
