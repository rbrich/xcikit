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

namespace xci::core {

using namespace core::log;


VfsFile VfsDirLoader::open(const std::string& path, std::ios_base::openmode mode)
{
    auto full_path = path_join(m_path, path);
    log_debug("VfsDirLoader: open file: {}", full_path);
    return VfsFile(std::move(full_path), mode);
}


Vfs& Vfs::default_instance()
{
    static Vfs instance;
    return instance;
}


void Vfs::mount(std::string real_path, std::string target_path)
{
    auto loader = std::make_unique<VfsDirLoader>(std::move(real_path));
    lstrip(target_path, '/');
    rstrip(target_path, '/');
    m_loaders.push_back({std::move(target_path), std::move(loader)});
}


VfsFile Vfs::open(std::string path, std::ios_base::openmode mode)
{
    lstrip(path, '/');
    log_debug("Vfs: try open: {}", path);
    for (auto& path_loader : m_loaders) {
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
