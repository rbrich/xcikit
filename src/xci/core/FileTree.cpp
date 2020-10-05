// FileTree.cpp created on 2020-10-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "FileTree.h"

namespace xci::core {

static constinit const char* s_default_ignore_list[] = {
    #if defined(__APPLE__)
        "/dev",
        "/System/Volumes",
    #elif defined(__linux__)
        "/dev",
        "/proc",
        "/sys",
        "/mnt",
        "/media",
    #endif
};

bool FileTree::is_default_ignored(const std::string& path)
{
    return std::any_of(std::begin(s_default_ignore_list), std::end(s_default_ignore_list),
            [&path](const char* ignore_path) { return path == ignore_path; });
}

} // namespace xci::core
