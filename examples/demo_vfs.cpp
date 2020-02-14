// demo_vfs.cpp created on 2018-09-01, part of XCI toolkit
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

#include <xci/core/Vfs.h>
#include <xci/core/log.h>
#include <xci/config.h>

using namespace xci::core;
using namespace xci::core::log;

int main()
{
    log_info("====== VFS with manually managed loaders ======");
    {
        Vfs vfs {Vfs::Loaders::NoArchives};
        vfs.add_loader(std::make_unique<vfs::RealDirectoryLoader>());
        vfs.mount("/does/not/exist");
        vfs.mount(XCI_SHARE);

        auto f = vfs.read_file("non/existent.file");
        log_info("demo: open result: {}", f.is_open());

        f = vfs.read_file("shaders/fps.frag.spv");
        log_info("demo: open result: {}", f.is_open());
        auto content = f.content();
        if (content)
            log_info("demo: file size: {}", content->size());
    }

    log_info("====== VFS with default loaders, load DAR archive ======");
    {
        // Buffer can outlive Vfs object.
        // DarArchive(VfsDirectory) will also be kept alive (but no longer accessible).
        BufferPtr content;
        {
            Vfs vfs {Vfs::Loaders::NoZip};

            // share.dar archive, generated during build by CMake
            vfs.mount(XCI_SHARE_DAR);
            // Directory overlapping the archive, will be tried after the archive
            vfs.mount(XCI_SHARE_DIR);
            auto f = vfs.read_file("fonts/Hack/Hack-Regular.ttf");
            content = f.content();
            // Vfs deleted here, together with DarArchiveLoader and VfsFile
            // but not the DarArchive or content Buffer
        }
        if (content)
            log_info("demo: file size: {}", content->size());
        // content Buffer and DarArchive deleted here
    }

    log_info("====== VFS with default loaders, load ZIP archive ======");
    {
        // Buffer can outlive Vfs object.
        BufferPtr content;
        {
            Vfs vfs;

            // share.zip archive, generated during build by CMake
            vfs.mount(XCI_SHARE_ZIP);
            auto f = vfs.read_file("fonts/Hack/Hack-Regular.ttf");
            content = f.content();
        }
        if (content)
            log_info("demo: file size: {}", content->size());
    }

    log_info("====== VFS leading slashes ======");
    {
        Vfs vfs {Vfs::Loaders::NoArchives};
        // Mount just a subfolder
        vfs.mount(XCI_SHARE_DIR "/shaders", "shaders");

        // Note that leading slashes in VFS paths don't matter (they are auto-normalized).
        // The VFS paths are always absolute, there is no CWD.
        // Same as above:
        //vfs.mount(XCI_SHARE_DIR "/shaders", "/shaders");
        // This applies to all VFS paths:
        auto f = vfs.read_file("/shaders/fps.frag.spv");
        log_info("demo: open result: {}", f.is_open());
        auto content = f.content();
        if (content)
            log_info("demo: file size: {}", content->size());
    }
    return 0;
}
