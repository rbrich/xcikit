// Vfs.cpp created on 2018-09-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Vfs.h"
#include "RealDirectory.h"
#include "DarArchive.h"
#include "WadArchive.h"
#include "ZipArchive.h"
#include <xci/core/log.h>
#include <xci/core/string.h>
#include <xci/core/sys.h>  // self_executable_path

#include <cstddef>  // byte

// Deprecated, but not easily replaceable - need to convert existing buffer to a stream, without copying
// Possible future C++ replacement is "spanstream",
// see http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2139r2.html#3.12
// GCC: pragma diagnostic doesn't disable the warning, let's use this ugly hack...
#define _BACKWARD_BACKWARD_WARNING_H 1  // NOLINT
#include <strstream>
#undef _BACKWARD_BACKWARD_WARNING_H

namespace xci::vfs {

using namespace core::log;


Vfs::Vfs(Loaders loaders)
{
    switch (loaders) {
        case Loaders::All:
            m_loaders.emplace_back(std::make_unique<ZipArchiveLoader>());
            [[fallthrough]];
        case Loaders::NoZip:
            m_loaders.emplace_back(std::make_unique<DarArchiveLoader>());
            m_loaders.emplace_back(std::make_unique<WadArchiveLoader>());
            [[fallthrough]];
        case Loaders::NoArchives:
            m_loaders.emplace_back(std::make_unique<RealDirectoryLoader>());
            [[fallthrough]];
        case Loaders::None:
            break;
    }
}


bool Vfs::mount(const fs::path& fs_path, std::string target_path)
{
    fs::path real_path;
    if (fs_path.is_relative()) {
        // handle relative path - it's relative to program executable,
        // or its parent, or its parent's parent. The nearest matched parent wins.
        auto base_dir = self_executable_path().parent_path();
        bool found = false;
        for (int parent = 0; parent < 5; ++parent) {
            std::error_code err;
            real_path = fs::canonical(base_dir / fs_path, err);
            if (!err) {
                // found an existing path
                found = true;
                break;
            }
            base_dir = base_dir.parent_path();
        }
        if (!found) {
            real_path = fs_path;  // keep it as relative path in respect to CWD
        }
    } else {
        real_path = fs_path;
    }

    std::shared_ptr<VfsDirectory> vfs_directory;
    const char* loader_name = nullptr;

    if (fs::is_directory(real_path)) {
        // Real directory - try each loader
        for (auto& loader : m_loaders) {
            if (!loader->can_load_fs_dir(real_path))
                continue;
            vfs_directory = loader->load_fs_dir(real_path);
            loader_name = loader->name();
            break;
        }
    } else {
        // Not directory - open as regular file
        auto f = std::make_unique<std::ifstream>(real_path, std::ios::binary);
        if (!*f) {
            log::error("Vfs: Failed to mount {}: {m}", real_path);
            return false;
        }
        // Try each loader
        for (auto& loader : m_loaders) {
            if (!loader->can_load_stream(*f))
                continue;
            vfs_directory = loader->load_stream(real_path.string(), std::move(f));
            loader_name = loader->name();
            break;
        }
    }

    if (!vfs_directory) {
        log::warning("Vfs: No loader found for {}", real_path);
        return false;
    }

    // Success, record the mounted dir
    lstrip(target_path, '/');
    rstrip(target_path, '/');
    log::info("Vfs: Mounted {} '{}' to /{}", loader_name, real_path, target_path);
    m_mounted_dir.push_back({std::move(target_path), std::move(vfs_directory)});
    return true;
}


bool Vfs::mount_memory(const std::byte* data, size_t size, std::string target_path)
{
    XCI_IGNORE_DEPRECATED(
    auto stream = std::make_unique<std::istrstream>((const char*)(data), size);
    )
    auto path = fmt::format("memory:{:x},{}", intptr_t(data), size);
    std::shared_ptr<VfsDirectory> vfs_directory;

    // Try each loader
    const char* loader_name = nullptr;
    for (auto& loader : m_loaders) {
        if (!loader->can_load_stream(*stream))
            continue;
        vfs_directory = loader->load_stream(std::string(path), std::move(stream));
        loader_name = loader->name();
        break;
    }

    if (!vfs_directory) {
        log::warning("Vfs: no loader found for {}", path);
        return false;
    }

    // Success, record the mounted dir
    lstrip(target_path, '/');
    rstrip(target_path, '/');
    log::info("Vfs: mounted {} '{}' to /{}", loader_name, path, target_path);
    m_mounted_dir.push_back({std::move(target_path), std::move(vfs_directory)});
    return true;
}


VfsFile Vfs::read_file(std::string path) const
{
    lstrip(path, '/');
    log::debug("Vfs: Try open: {}", path);
    for (const auto& path_loader : m_mounted_dir) {
        // Is the loader applicable for requested path?
        auto spath = path;  // path relative to mount point
        if (!path_loader.path.empty()) {
            if (!remove_prefix(spath, path_loader.path))
                continue;
            if (spath.front() != '/')
                continue;
            lstrip(spath, '/');
        }
        // Open the path with loader
        log::debug("Vfs: Trying {} mounted at /{}", path_loader.vfs_dir->type(), path_loader.path);
        auto f = path_loader.vfs_dir->read_file(spath);
        if (f.is_open())
            return f;
    }
    log::error("Vfs: File not found: {}", path);
    return {};
}


}  // namespace xci::vfs
