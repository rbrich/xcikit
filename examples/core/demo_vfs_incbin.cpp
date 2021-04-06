// demo_vfs.cpp created on 2018-09-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

/// Shows how to embed the VFS archive in the program binary

#include <xci/core/Vfs.h>
#include <xci/core/log.h>

#define INCBIN_PREFIX g_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include <incbin.h>

// SHARE_ARCHIVE is passed as -D arg from CMake,
// it contains absolute path to generated "share.dar" archive
INCBIN(share_file, SHARE_ARCHIVE);

using namespace xci::core;
using namespace xci::core::log;

int main()
{
    Vfs vfs;
    vfs.mount_memory((const std::byte*) g_share_file_data, g_share_file_size);

    auto f = vfs.read_file("script/std.fire");
    info("demo: open result: {}", f.is_open());
    auto content = f.content();
    if (content)
        info("demo: file size: {}", content->size());

    return 0;
}
