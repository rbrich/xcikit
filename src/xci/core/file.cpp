// file.cpp created on 2018-03-29 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "file.h"
#include <xci/compat/unistd.h>
#include <xci/core/log.h>

#include <fstream>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstddef>  // byte

namespace xci::core {


std::optional<std::string> read_text_file(const fs::path& pathname)
{
    std::ifstream f(pathname);
    return read_text_file(f);
}


std::optional<std::string> read_text_file(std::istream& stream)
{
    if (!stream)
        return {};

    stream.seekg(0, std::ios::end);
    auto file_size = size_t(stream.tellg());
    stream.seekg(0, std::ios::beg);
    std::string content(file_size, char(0));
    stream.read(&content[0], content.size());
    if (!stream)
        content.clear();

    return content;
}


BufferPtr read_binary_file(const fs::path& pathname)
{
    std::ifstream f(pathname, std::ios::binary);
    return read_binary_file(f);
}


BufferPtr read_binary_file(std::istream& stream)
{
    if (!stream)
        return {};

    stream.seekg(0, std::ios::end);
    auto file_size = size_t(stream.tellg());
    stream.seekg(0, std::ios::beg);

    auto* content = new std::byte[file_size];
    stream.read(reinterpret_cast<char*>(content), file_size);
    if (!stream) {
        delete[] content;
        return {};
    }

    return {new Buffer{content, file_size},
            [](Buffer* b){ delete[] b->data(); delete b; }};
}


bool write(int fd, std::string_view s)
{
    while (!s.empty()) {
        ssize_t r = ::write(fd, s.data(), s.size());
        if (r == -1) {
            if (errno == EINTR)
                continue;
            log::error("write({}, {} bytes): {m}", fd, s.size());
            return false;
        }
        assert(r > 0);
        if (r == 0)
            return false;
        s = s.substr(r);
    }
    return true;
}


}  // namespace xci::core
