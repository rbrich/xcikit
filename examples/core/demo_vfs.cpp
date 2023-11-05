// demo_vfs.cpp created on 2018-09-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/vfs/Vfs.h>
#include <xci/core/log.h>
#include <xci/config.h>

using namespace xci::core;
using namespace xci::core::log;

int main()
{
    info("====== VFS with manually managed loaders ======");
    {
        Vfs vfs {Vfs::Loaders::NoArchives};
        vfs.add_loader(std::make_unique<vfs::RealDirectoryLoader>());
        vfs.mount("/does/not/exist");
        vfs.mount(XCI_SHARE_DIR);

        auto f = vfs.read_file("non/existent.file");
        info("demo: open result: {}", f.is_open());

        f = vfs.read_file("shaders/fps.frag.spv");
        info("demo: open result: {}", f.is_open());
        auto content = f.content();
        if (content)
            info("demo: file size: {}", content->size());
    }

    info("====== VFS with default loaders, load DAR archive ======");
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
            info("demo: file size: {}", content->size());
        // content Buffer and DarArchive deleted here
    }

    info("====== VFS with default loaders, load ZIP archive ======");
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
            info("demo: file size: {}", content->size());
    }

    info("====== VFS leading slashes ======");
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
        info("demo: open result: {}", f.is_open());
        auto content = f.content();
        if (content)
            info("demo: file size: {}", content->size());
    }
    return 0;
}
