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
#include <xci/util/log.h>

namespace xci {
namespace util {

using namespace util::log;


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


void Vfs::mount_dir(std::string path)
{
    m_loaders.push_back(std::make_unique<VfsDirLoader>(std::move(path)));
}


VfsFile Vfs::open(const std::string& path, std::ios_base::openmode mode)
{
    log_debug("Vfs: try open: {}", path);
    for (auto& loader : m_loaders) {
        auto f = loader->open(path, mode);
        if (f.is_open()) {
            log_debug("Vfs: success!");
            return f;
        }
    }
    log_debug("Vfs: failed to open file");
    return {};
}


}}  // namespace xci::util
